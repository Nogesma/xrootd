/*
 * XrdEcReader.hh
 *
 *  Created on: 18 Dec 2020
 *      Author: simonm
 */

#ifndef SRC_XRDEC_XRDECREADER_HH_
#define SRC_XRDEC_XRDECREADER_HH_

#include "XrdEc/XrdEcObjCfg.hh"

#include "XrdCl/XrdClFileOperations.hh"

#include "XrdZip/XrdZipLFH.hh"
#include "XrdZip/XrdZipCDFH.hh"
#include "XrdZip/XrdZipUtils.hh"

#include "XrdOuc/XrdOucCRC32C.hh"

#include <string>
#include <tuple>
#include <unordered_map>
#include <algorithm>
#include <iterator>
#include <list>

namespace XrdEc
{

  class Reader
  {
    //----------------------------------------------------------------------------
    //! OpenOnly operation (@see ZipOperation) - a private ZIP operation
    //----------------------------------------------------------------------------
    template<bool HasHndl>
    class OpenOnlyImpl: public XrdCl::ZipOperation<OpenOnlyImpl, HasHndl,
        XrdCl::Resp<void>, XrdCl::Arg<std::string>>
    {
      public:

        //------------------------------------------------------------------------
        //! Inherit constructors from FileOperation (@see FileOperation)
        //------------------------------------------------------------------------
        using XrdCl::ZipOperation<OpenOnlyImpl, HasHndl, XrdCl::Resp<void>,
            XrdCl::Arg<std::string>>::ZipOperation;

        //------------------------------------------------------------------------
        //! Argument indexes in the args tuple
        //------------------------------------------------------------------------
        enum { UrlArg };

        //------------------------------------------------------------------------
        //! @return : name of the operation (@see Operation)
        //------------------------------------------------------------------------
        std::string ToString()
        {
          return "OpenOnly";
        }

      protected:

        //------------------------------------------------------------------------
        //! RunImpl operation (@see Operation)
        //!
        //! @param params :  container with parameters forwarded from
        //!                  previous operation
        //! @return       :  status of the operation
        //------------------------------------------------------------------------
        XrdCl::XRootDStatus RunImpl( XrdCl::PipelineHandler *handler,
                                     uint16_t                pipelineTimeout )
        {
          std::string      url     = std::get<UrlArg>( this->args ).Get();
          uint16_t         timeout = pipelineTimeout < this->timeout ?
                                     pipelineTimeout : this->timeout;
          return this->zip->OpenOnly( url, handler, timeout );
        }
    };

    //----------------------------------------------------------------------------
    //! Factory for creating OpenArchiveImpl objects
    //----------------------------------------------------------------------------
    inline OpenOnlyImpl<false> OpenOnly( XrdCl::Ctx<XrdCl::ZipArchive> zip,
                                         XrdCl::Arg<std::string>       fn,
                                         uint16_t                      timeout = 0 )
    {
      return OpenOnlyImpl<false>( std::move( zip ), std::move( fn ) ).Timeout( timeout );
    }

    //-------------------------------------------------------------------------
    // Buffer for a single chunk of data
    //-------------------------------------------------------------------------
    typedef std::vector<char> buffer_t;
    //-------------------------------------------------------------------------
    // Read callback, to be called with status and number of bytes read
    //-------------------------------------------------------------------------
    typedef std::function<void( const XrdCl::XRootDStatus&, buffer_t&& )> callback_t;

    struct CacheEntry
    {
      CacheEntry( ObjCfg &objcfg ) : objcfg( objcfg ), evict( false ), blknb( 0 ), strpnb( 0 ), state( Empty )
      {
      }

      void Read( size_t                                             offset,
                 size_t                                             size,
                 char                                              *usrbuff,
                 std::function<void( const XrdCl::XRootDStatus& )>  callback )
      {
        std::unique_lock<std::mutex> lck( mtx );
        if( state != Valid )
        {
          callbacks.emplace_back( offset, size, usrbuff, std::move( callback ) );
          return;
        }
        // if there was an error, forward it to the callback
        if( !status.IsOK() )
        {
          lck.unlock();
          callback( status );
          return;
        }
        // otherwise copy the data into user buffer
        memcpy( usrbuff, buffer.data() + offset, size );
        lck.unlock();
        callback( status );
      }

      void Load( size_t blknb, size_t strpnb )
      {
        std::unique_lock<std::mutex> lck( mtx );
        if( state != Empty ) return;
        this->blknb  = blknb;
        this->strpnb = strpnb;
        // TODO load the data from remote endpoint and verify integrity
        //      if necessary run recovery

      }

      enum state_t { Empty = 0, Loading, Valid };


      typedef std::function<void( const XrdCl::XRootDStatus& )> callback_t;
      typedef std::tuple<size_t, size_t, char*, callback_t>     args_t;

      ObjCfg             &objcfg;
      buffer_t            buffer;
      bool                evict;
      size_t              blknb;
      size_t              strpnb;
      state_t             state;
      XrdCl::XRootDStatus status;
      std::list<args_t>   callbacks;
      std::mutex          mtx;
    };


    struct ReadCache
    {
      ReadCache( ObjCfg &objcfg ): objcfg( objcfg ) { }

//      CacheEntry& Get( size_t blknb, size_t strpnb )
//      {
//        auto itr = cache.find( blknb );
//        if( itr == cache.end() )
//        {
//          itr = cache.emplace( blknb ).first;
//          itr->second.resize( objcfg.nbchunks, objcfg );
//        }
//        std::vector<CacheEntry> &block = itr->second;
//        block[strpnb].Load( blknb, strpnb );
//        return block[strpnb];
//      }

      typedef std::unordered_map<size_t, std::vector<CacheEntry>> cache_t;

      ObjCfg  &objcfg;
      cache_t  cache;
    };

    public:
      Reader( ObjCfg &objcfg ) : objcfg( objcfg )
      {
      }

      virtual ~Reader()
      {
      }

      void Open( XrdCl::ResponseHandler *handler )
      {
        const size_t size = objcfg.plgr.size();
        std::vector<XrdCl::Pipeline> opens; opens.reserve( size );
        for( size_t i = 0; i < size; ++i )
        {
          // generate the URL
          std::string url = objcfg.plgr[i] + objcfg.obj + ".zip";
          // create the file object
          dataarchs.emplace( url, std::make_shared<XrdCl::ZipArchive>() );
          // open the archive
          opens.emplace_back( OpenOnly( *dataarchs[url], url ) );
        }
        // in parallel open the data files and read the metadata
        XrdCl::Pipeline p = XrdCl::Parallel( ReadMetadata( 0 ), XrdCl::Parallel( opens ) ) >>
                              [=]( XrdCl::XRootDStatus &st )
                              { // set the central directories in ZIP archives
                                auto itr = dataarchs.begin();
                                for( ; itr != dataarchs.end() ; ++itr )
                                {
                                  const std::string &url    = itr->first;
                                  auto              &zipptr = itr->second;
                                  if( zipptr->openstage == XrdCl::ZipArchive::NotParsed )
                                    zipptr->SetCD( metadata[url] );
                                  auto itr = zipptr->cdmap.begin();
                                  for( ; itr != zipptr->cdmap.end() ; ++itr )
                                    urlmap.emplace( itr->first, url );
                                }
                                metadata.clear();
                                // call user handler
                                if( handler )
                                  handler->HandleResponse( new XrdCl::XRootDStatus( st ), nullptr );
                              };
        XrdCl::Async( std::move( p ) );
      }

      void Read( uint64_t offset, uint32_t length, void *buffer, XrdCl::ResponseHandler *handler )
      {
        // TODO
      }

    private:

      void Read( size_t blknb, size_t strpnb, callback_t cb )
      {
        // generate the file name (blknb/strpnb)
        std::string fn = objcfg.obj + '.' + std::to_string( blknb ) + '.' + std::to_string( strpnb );
        // if the block/stripe does not exist it means we are reading passed the end of the file
        auto itr = urlmap.find( fn );
        if( itr == urlmap.end() ) return cb( XrdCl::XRootDStatus(), buffer_t() );
        // get the URL of the ZIP archive with the respective data
        const std::string &url = itr->second;
        // get the ZipArchive object
        auto &zipptr = dataarchs[url];
        // check the size of the data to be read
        XrdCl::StatInfo *info = nullptr;
        auto st = zipptr->Stat( fn, info );
        if( !st.IsOK() ) return cb( st, buffer_t() );
        uint32_t rdsize = info->GetSize();
        delete info;
        // create a buffer for the data
        auto rdbuff = std::make_shared<buffer_t>( rdsize, 0 );
        // issue the read request
        XrdCl::Async( XrdCl::ReadFrom( *zipptr, fn, 0, rdsize, rdbuff->data() ) >>
                        [cb, rdbuff]( XrdCl::XRootDStatus &st, XrdCl::ChunkInfo &ch )
                        {
                          cb( st, std::move( *rdbuff ) );
                        } );
      }

      XrdCl::Pipeline ReadMetadata( size_t index )
      {
        const size_t size = objcfg.plgr.size();
        // create the File object
        auto file = std::make_shared<XrdCl::File>();
        // prepare the URL for Open operation
        std::string url = objcfg.plgr[index] + objcfg.obj + ".metadata.zip";
        // arguments for the Read operation
        XrdCl::Fwd<uint32_t> rdsize;
        XrdCl::Fwd<void*>    rdbuff;

        return XrdCl::Open( *file, url, XrdCl::OpenFlags::Read ) >>
                 [=]( XrdCl::XRootDStatus &st, XrdCl::StatInfo &info )
                 {
                   if( !st.IsOK() )
                   {
                     if( index + 1 < size )
                       XrdCl::Pipeline::Replace( ReadMetadata( index + 1 ) );
                     return;
                   }
                   // prepare the args for the subsequent operation
                   rdsize = info.GetSize();
                   rdbuff = new char[info.GetSize()];
                 }
             | XrdCl::Read( *file, 0, rdsize, rdbuff ) >>
                 [=]( XrdCl::XRootDStatus &st, XrdCl::ChunkInfo &ch )
                 {
                   if( !st.IsOK() )
                   {
                     if( index + 1 < size )
                       XrdCl::Pipeline::Replace( ReadMetadata( index + 1 ) );
                     return;
                   }
                   // now parse the metadata
                   if( !ParseMetadata( ch ) )
                   {
                     if( index + 1 < size )
                       XrdCl::Pipeline::Replace( ReadMetadata( index + 1 ) );
                     return;
                   }
                 }
             | XrdCl::Final(
                 [=]( const XrdCl::XRootDStatus& )
                 {
                   // deallocate the buffer if necessary
                   if( rdbuff.Valid() )
                   {
                     char* buffer = reinterpret_cast<char*>( *rdbuff );
                     delete[] buffer;
                   }
                   // close the file if necessary (we don't really care about the result)
                   if( file->IsOpen() )
                     XrdCl::Async( XrdCl::Close( *file ) >> [file]( XrdCl::XRootDStatus& ){ } );
                 } );
      }

      bool ParseMetadata( XrdCl::ChunkInfo &ch )
      {
        const size_t mincnt = objcfg.nbdata + objcfg.nbparity;
        const size_t maxcnt = objcfg.plgr.size();

        char   *buffer = reinterpret_cast<char*>( ch.buffer );
        size_t  length = ch.length;

        for( size_t i = 0; i < maxcnt; ++i )
        {
          uint32_t signature = XrdZip::to<uint32_t>( buffer );
          if( signature != XrdZip::LFH::lfhSign )
          {
            if( i + 1 < mincnt ) return false;
            break;
          }
          XrdZip::LFH lfh( buffer );
          // check if we are not reading passed the end of the buffer
          if( lfh.lfhSize + lfh.uncompressedSize > length ) return false;
          buffer += lfh.lfhSize;
          length -= lfh.lfhSize;
          // verify the checksum
          uint32_t crc32val = crc32c( 0, buffer, lfh.uncompressedSize );
          if( crc32val != lfh.ZCRC32 ) return false;
          // keep the metadata
          metadata.emplace( lfh.filename, buffer_t( buffer, buffer + lfh.uncompressedSize ) );
          buffer += lfh.uncompressedSize;
          length -= lfh.uncompressedSize;
        }

        return true;
      }

      typedef std::unordered_map<std::string, std::shared_ptr<XrdCl::ZipArchive>> dataarchs_t;
      typedef std::unordered_map<std::string, buffer_t> metadata_t;
      typedef std::unordered_map<std::string, std::string> urlmap_t;

      ObjCfg      &objcfg;
      dataarchs_t  dataarchs; //> map URL to ZipArchive object
      metadata_t   metadata;  //> map URL to CD metadata
      urlmap_t     urlmap;    //> map blknb/strpnb (data chunk) to URL
  };

} /* namespace XrdEc */

#endif /* SRC_XRDEC_XRDECREADER_HH_ */
