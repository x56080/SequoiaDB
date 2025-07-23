/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = RestParamDefine.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoias3.common;

public class RestParamDefine {
    public static final String AUTHORIZATION           = "authorization";

    public static final String VERSIONS                = "versions";
    public static final String VERSION_ID              = "versionId";
    public static final String VERSIONING              = "versioning";
    public static final String LOCATION                = "location";
    public static final String DELIMITER               = "delimiter-config";

    public static final int    MAX_KEYS_DEFAULT        = 1000;

    public static class UserPara{
        public static final String CREATE_USER             = "Action=CreateUser";
        public static final String CREATE_ACCESSKEY        = "Action=CreateAccessKey";
        public static final String GET_ACCESSKEY           = "Action=GetAccessKey";
        public static final String DELETE_USER             = "Action=DeleteUser";
        public static final String USER_NAME               = "UserName";
        public static final String ROLE                    = "Role";
        public static final String FORCE                   = "Force";
    }

    public static class ListObjectsPara{
        public static final String LIST_TYPE2              = "list-type=2";
        public static final String PREFIX                  = "prefix";
        public static final String DELIMITER               = "delimiter";
        public static final String CONTINUATIONTOKEN       = "continuation-token";
        public static final String START_AFTER             = "start-after";
        public static final String MAX_KEYS                = "max-keys";
        public static final String ENCODING_TYPE           = "encoding-type";
        public static final String FETCH_OWNER             = "fetch-owner";
    }

    public static class ListObjectVersionsPara{
        public static final String PREFIX                  = "prefix";
        public static final String DELIMITER               = "delimiter";
        public static final String KEY_MARKER              = "key-marker";
        public static final String VERSION_ID_MARKER       = "version-id-marker";
        public static final String MAX_KEYS                = "max-keys";
        public static final String ENCODING_TYPE           = "encoding-type";
    }

    public static class PutObjectHeader {
        public static final String CONTENT_LENGTH       = "content-length";
        public static final String CONTENT_MD5          = "content-md5";
        public static final String DATE                 = "date";
        public static final String X_AMZ_DATE           = "x-amz-date";
        public static final String CONTENT_ENCODING     = "content-encoding";
        public static final String CONTENT_TYPE         = "content-type";
        public static final String X_AMZ_META_PREFIX    = "x-amz-meta-";
        public static final String CACHE_CONTROL        = "cache-control";
        public static final String CONTENT_DISPOSITION  = "content-disposition";
        public static final String EXPIRES              = "expires";
        public static final String CONTENT_LANGUAGE     = "content-language";
    }

    public static class PutObjectResultHeader {
        public static final String ETAG         = "ETag";
        public static final String VERSION_ID   = "x-amz-version-id";
    }

    public static class GetObjectReqPara{
        public static final String RES_CONTENT_TYPE         = "response-content-type";
        public static final String RES_CONTENT_LANGUAGE     = "response-content-language";
        public static final String RES_EXPIRES              = "response-expires";
        public static final String RES_CACHE_CONTROL        = "response-cache-control";
        public static final String RES_CONTENT_DISPOSITION  = "response-content-disposition";
        public static final String RES_CONTENT_ENCODING     = "response-content-encoding";
    }

    public static class GetObjectReqHeader{
        public static final String REQ_RANGE                = "range";
        public static final String REQ_IF_MODIFIED_SINCE    = "if-modified-since";
        public static final String REQ_IF_UNMODIFIED_SINCE  = "if-unmodified-since";
        public static final String REQ_IF_MATCH             = "if-match";
        public static final String REQ_IF_NONE_MATCH        = "if-none-match";
    }

    public static class GetObjectResHeader{
        public static final String CONTENT_ENCODING        = "Content-Encoding";
        public static final String CONTENT_TYPE            = "Content-Type";
        public static final String CACHE_CONTROL           = "Cache-Control";
        public static final String CONTENT_DISPOSITION     = "Content-Disposition";
        public static final String EXPIRES                 = "Expires";
        public static final String CONTENT_LANGUAGE        = "Content-Language";
        public static final String LAST_MODIFIED           = "Last-Modified";
        public static final String CONTENT_LENGTH          = "Content-Length";
        public static final String ETAG                    = "ETag";
        public static final String CONTENT_RANGE           = "Content-Range";
        public static final String VERSION_ID              = "x-amz-version-id";
        public static final String DELETE_MARKER           = "x-amz-delete-marker";
        public static final String ACCEPT_RANGES           = "Accept-Ranges";
    }

    public static class DeleteObjectResultHeader {
        public static final String  VERSION_ID    = "x-amz-version-id";
        public static final String  DELETE_MARKER = "x-amz-delete-marker";
    }

    public static class HeadBucketResultHeader {
        public static final String REGION = "x-amz-bucket-region";
    }

    public static final int KEY_LENGTH              = 900;
    public static final int X_AMZ_META_LENGTH       = 2*1024;

    public static final String REST_CREDENTIAL      = "Credential=";

    public static final String REST_DELIMITER       = "/";
    public static final String REST_S3              = "/s3";
    public static final String REST_USERS           = "/users";
    public static final String REST_REGION          = "/region";

    public static final String REST_RANGE_START     = "bytes=";
    public static final String REST_HYPHEN          = "-";

    public static final String ENCODING_TYPE_URL    = "url";
}
