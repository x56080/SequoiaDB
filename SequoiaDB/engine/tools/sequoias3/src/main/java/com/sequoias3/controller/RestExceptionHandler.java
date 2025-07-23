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

   Source File Name = RestExceptionHandler.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoias3.controller;

import com.sequoias3.core.Error;
import com.sequoias3.exception.S3ServerException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.ControllerAdvice;
import org.springframework.web.bind.annotation.ExceptionHandler;
import org.springframework.web.bind.annotation.ResponseBody;

import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

@ControllerAdvice
public class RestExceptionHandler {
    private static final Logger logger = LoggerFactory.getLogger(RestExceptionHandler.class);
    private static final String ERROR_ATTRIBUTE = "X-S3-ERROR";

    @ExceptionHandler(S3ServerException.class)
    @ResponseBody
    public ResponseEntity s3ExceptionHandler(S3ServerException e, HttpServletRequest request,
                                             HttpServletResponse response) {
        String msg = String.format("request=%s, errcode=%s, message=%s", request.getRequestURI(), e.getError()
                .getCode(), e.getMessage());
        logger.error(msg, e);

        HttpStatus status;
        switch (e.getError()) {
            case OBJECT_IF_NONE_MATCH_FAILED:
            case OBJECT_IF_MODIFIED_SINCE_FAILED:
                status = HttpStatus.NOT_MODIFIED;
                break;
            case INVALID_ARGUMENT:
            case USER_CREATE_NAME_INVALID:
            case USER_CREATE_ROLE_INVALID:
            case NO_CREDENTIALS:
            case BUCKET_INVALID_BUCKETNAME:
            case BUCKET_TOO_MANY_BUCKETS:
            case BUCKET_INVALID_VERSIONING_STATUS:
            case BUCKET_INVALID_LOCATION:
            case OBJECT_KEY_TOO_LONG:
            case OBJECT_METADATA_TOO_LARGE:
            case OBJECT_INVALID_TOKEN:
            case OBJECT_BAD_DIGEST:
            case OBJECT_INVALID_KEY:
            case OBJECT_INVALID_DIGEST:
            case OBJECT_INVALID_VERSION:
            case OBJECT_INVALID_RANGE:
            case REGION_INVALID_SHARDINGTYPE:
            case REGION_INVALID_LOBPAGESIZE:
            case REGION_INVALID_REPLSIZE:
            case REGION_LOCATION_NULL:
            case REGION_LOCATION_SAME:
            case REGION_LOCATION_SPLIT:
            case REGION_LOCATION_EXIST:
            case REGION_INVALID_DOMAIN:
            case REGION_INVALID_REGIONNAME:
            case BUCKET_NO_LIST_OBJECTS_V1:
                status = HttpStatus.BAD_REQUEST;
                break;
            case INVALID_ACCESSKEYID:
            case INVALID_ADMINISTRATOR:
            case USER_DELETE_INIT_ADMIN:
            case SIGNATURE_NOT_MATCH:
            case ACCESS_DENIED:
                status = HttpStatus.FORBIDDEN;
                break;
            case USER_NOT_EXIST:
            case BUCKET_NOT_EXIST:
            case OBJECT_NO_SUCH_KEY:
            case OBJECT_NO_SUCH_VERSION:
            case REGION_NO_SUCH_REGION:
                status = HttpStatus.NOT_FOUND;
                break;
            case METHOD_NOT_ALLOWED:
                status = HttpStatus.METHOD_NOT_ALLOWED;
                break;
            case USER_CREATE_EXIST:
            case BUCKET_ALREADY_EXIST:
            case BUCKET_ALREADY_OWNED_BY_YOU:
            case BUCKET_NOT_EMPTY:
            case OBJECT_IS_IN_USE:
            case REGION_NOT_EMPTY:
            case REGION_CONFLICT_TYPE:
            case REGION_CONFLICT_DOMAIN:
            case REGION_CONFLICT_LOCATION:
            case REGION_CONFLICT_LOBPAGESIZE:
            case REGION_CONFLICT_REPLSIZE:
            case BUCKET_DELIMITER_NOT_STABLE:
                status = HttpStatus.CONFLICT;
                break;
            case OBJECT_IF_MATCH_FAILED:
            case OBJECT_IF_UNMODIFIED_SINCE_FAILED:
                status = HttpStatus.PRECONDITION_FAILED;
                break;
            case OBJECT_RANGE_NOT_SATISFIABLE:
                status = HttpStatus.REQUESTED_RANGE_NOT_SATISFIABLE;
                break;
            default:
                status = HttpStatus.INTERNAL_SERVER_ERROR;
        }

        Error exceptionBody = new Error(e, request.getRequestURI());
        if ("HEAD".equalsIgnoreCase(request.getMethod())){
            return ResponseEntity.status(status).build();
        } else {
            return ResponseEntity.status(status).body(exceptionBody);
        }
    }

    @ExceptionHandler(Exception.class)
    public ResponseEntity unexpectedExceptionHandler(Exception e, HttpServletRequest request,
                                                     HttpServletResponse response) throws Exception {
        String msg = String.format("request=%s", request.getRequestURI());
        logger.error(msg, e);

        HttpStatus status = HttpStatus.INTERNAL_SERVER_ERROR;

        Error exceptionBody = new Error(e, request.getRequestURI());
        if ("HEAD".equalsIgnoreCase(request.getMethod())) {
            String error = exceptionBody.toString();
            response.setHeader(ERROR_ATTRIBUTE, error);
            return ResponseEntity.status(status).build();
        } else {
            return ResponseEntity.status(status).body(exceptionBody);
        }
    }
}