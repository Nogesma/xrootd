//
// Created by segransm on 11/14/23.
//

#ifndef XROOTD_XRDS3ERRORRESPONSE_HH
#define XROOTD_XRDS3ERRORRESPONSE_HH

#include <map>
#include <string>
#include <utility>

namespace S3 {

struct S3ErrorCode {
  std::string code;
  std::string description;
  int httpCode;
};

enum class S3Error {
  None,
  AccessControlListNotSupported,
  AccessDenied,
  AccessPointAlreadyOwnedByYou,
  AccountProblem,
  AllAccessDisabled,
  AmbiguousGrantByEmailAddress,
  AuthorizationHeaderMalformed,
  BadDigest,
  BucketAlreadyExists,
  BucketAlreadyOwnedByYou,
  BucketNotEmpty,
  ClientTokenConflict,
  CredentialsNotSupported,
  CrossLocationLoggingProhibited,
  EntityTooSmall,
  EntityTooLarge,
  ExpiredToken,
  IllegalLocationConstraintException,
  IllegalVersioningConfigurationException,
  IncompleteBody,
  IncorrectNumberOfFilesInPostRequest,
  InlineDataTooLarge,
  InternalError,
  InvalidAccessKeyId,
  InvalidAccessPoint,
  InvalidAccessPointAliasError,
  InvalidAddressingHeader,
  InvalidArgument,
  InvalidBucketAclWithObjectOwnership,
  InvalidBucketName,
  InvalidBucketState,
  InvalidDigest,
  InvalidEncryptionAlgorithmError,
  InvalidLocationConstraint,
  InvalidObjectState,
  InvalidPart,
  InvalidPartOrder,
  InvalidPayer,
  InvalidPolicyDocument,
  InvalidRange,
  InvalidRequest,
  InvalidSecurity,
  InvalidSOAPRequest,
  InvalidStorageClass,
  InvalidTargetBucketForLogging,
  InvalidToken,
  InvalidURI,
  KeyTooLongError,
  MalformedACLError,
  MalformedPOSTRequest,
  MalformedXML,
  MaxMessageLengthExceeded,
  MaxPostPreDataLengthExceededError,
  MetadataTooLarge,
  MethodNotAllowed,
  MissingAttachment,
  MissingContentLength,
  MissingRequestBodyError,
  MissingSecurityElement,
  MissingSecurityHeader,
  NoLoggingStatusForKey,
  NoSuchBucket,
  NoSuchBucketPolicy,
  NoSuchCORSConfiguration,
  NoSuchKey,
  NoSuchLifecycleConfiguration,
  NoSuchMultiRegionAccessPoint,
  NoSuchWebsiteConfiguration,
  NoSuchTagSet,
  NoSuchUpload,
  NoSuchVersion,
  NotImplemented,
  NotModified,
  NotSignedUp,
  OwnershipControlsNotFoundError,
  OperationAborted,
  PermanentRedirect,
  PreconditionFailed,
  Redirect,
  RequestHeaderSectionTooLarge,
  RequestIsNotMultiPartContent,
  RequestTimeout,
  RequestTimeTooSkewed,
  RequestTorrentOfBucketError,
  RestoreAlreadyInProgress,
  ServerSideEncryptionConfigurationNotFoundError,
  ServiceUnavailable,
  SignatureDoesNotMatch,
  SlowDown,
  TemporaryRedirect,
  TokenRefreshRequired,
  TooManyAccessPoints,
  TooManyBuckets,
  TooManyMultiRegionAccessPointregionsError,
  TooManyMultiRegionAccessPoints,
  UnexpectedContent,
  UnresolvableGrantByEmailAddress,
  UserKeyMustBeSpecified,
  NoSuchAccessPoint,
  InvalidTag,
  MalformedPolicy,
  // S3 Error

  XAmzContentSHA256Mismatch,
  // XrdErrors
  InvalidObjectName,
  ObjectExistAsDir,
  ObjectExistInObjectPath,
};

// todo: description
const std::map<S3Error, S3ErrorCode> S3ErrorMap = {
    {S3Error::NotImplemented,
     {.code = "NotImplemented", .description = "", .httpCode = 501}},
    {S3Error::MissingContentLength,
     {.code = "MissingContentLength", .description = "", .httpCode = 411}},
    {S3Error::IncompleteBody,
     {.code = "IncompleteBody", .description = "", .httpCode = 400}},
    {S3Error::InternalError,
     {.code = "InternalError", .description = "", .httpCode = 500}},
    {S3Error::BucketNotEmpty,
     {.code = "BucketNotEmpty", .description = "", .httpCode = 409}},
    {S3Error::BadDigest,
     {.code = "BadDigest", .description = "", .httpCode = 400}},
    {S3Error::AccessDenied,
     {.code = "AccessDenied", .description = "", .httpCode = 403}},
    {S3Error::InvalidDigest,
     {.code = "InvalidDigest", .description = "", .httpCode = 400}},
    {S3Error::InvalidRequest,
     {.code = "InvalidRequest", .description = "", .httpCode = 400}},
    {S3Error::BucketAlreadyOwnedByYou,
     {.code = "BucketAlreadyOwnedByYou", .description = "", .httpCode = 409}},
    {S3Error::InvalidURI,
     {.code = "InvalidURI", .description = "", .httpCode = 400}},
    {S3Error::InvalidObjectName,
     {.code = "InvalidObjectName",
      .description = "Object name is not valid",
      .httpCode = 400}},
    {S3Error::ObjectExistAsDir,
     {.code = "ObjectExistAsDir",
      .description = "A directory already exist with this path",
      .httpCode = 400}},
    {S3Error::ObjectExistInObjectPath,
     {.code = "ObjectExistInObjectPath",
      .description = "An object already exist in the object path",
      .httpCode = 400}},
    {S3Error::NoSuchKey,
     {.code = "NoSuchKey",
      .description = "Object does not exist",
      .httpCode = 404}},
    {S3Error::InvalidBucketName,
     {.code = "InvalidBucketName",
      .description = "Bucket name is not valid",
      .httpCode = 400}},
    {S3Error::InvalidArgument,
     {.code = "InvalidArgument", .description = "", .httpCode = 400}},
    {S3Error::NoSuchBucket,
     {.code = "NoSuchBucket", .description = "", .httpCode = 404}},
    {S3Error::OperationAborted,
     {.code = "OperationAborted", .description = "", .httpCode = 404}},
    {S3Error::BucketAlreadyExists,
     {.code = "BucketAlreadyExists", .description = "", .httpCode = 409}},
    {S3Error::MalformedXML,
     {.code = "MalformedXML", .description = "", .httpCode = 400}},
    {S3Error::PreconditionFailed,
     {.code = "PreconditionFailed", .description = "", .httpCode = 412}},
    {S3Error::NotModified,
     {.code = "NotModified", .description = "", .httpCode = 304}},
    {S3Error::SignatureDoesNotMatch,
     {.code = "SignatureDoesNotMatch", .description = "", .httpCode = 403}},
    {S3Error::InvalidAccessKeyId,
     {.code = "InvalidAccessKeyId", .description = "", .httpCode = 403}},
    {S3Error::NoSuchAccessPoint,
     {.code = "NoSuchAccessPoint", .description = "", .httpCode = 404}},
    {S3Error::XAmzContentSHA256Mismatch,
     {.code = "XAmzContentSHA256Mismatch", .description = "", .httpCode = 400}},
    {S3Error::NoSuchUpload,
     {.code = "NoSuchUpload", .description = "", .httpCode = 404}},
    {S3Error::InvalidPart,
     {.code = "InvalidPart", .description = "", .httpCode = 400}},
    {S3Error::InvalidPartOrder,
     {.code = "InvalidPartOrder", .description = "", .httpCode = 400}},
};

}  // namespace S3

#endif  // XROOTD_XRDS3ERRORRESPONSE_HH