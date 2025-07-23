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
import com.sequoias3.utils.RestUtils;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
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

    @Autowired
    RestUtils restUtils;

    @ExceptionHandler(S3ServerException.class)
    @ResponseBody
    public ResponseEntity s3ExceptionHandler(S3ServerException e, HttpServletRequest request,
                                             HttpServletResponse response) {
        String msg = String.format("request=%s, errcode=%s, message=%s", request.getRequestURI(), e.getError()
                .getCode(), e.getMessage());
        logger.error(msg, e);

        HttpStatus status = restUtils.convertStatus(e);

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