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

   Source File Name = RestExceptionHandlerBase.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/01/2026  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.schedule.exception;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.ExceptionHandler;
import org.springframework.web.bind.annotation.ResponseBody;

import javax.servlet.http.HttpServletRequest;

/**
 * 简化版 RestExceptionHandlerBase
 * 仅保留核心异常处理与统一 JSON 响应
 */
public abstract class RestExceptionHandlerBase {

    private static final Logger logger = LoggerFactory.getLogger(RestExceptionHandlerBase.class);

    @ExceptionHandler(Exception.class)
    @ResponseBody
    public ResponseEntity<ExceptionBody> handleException(Exception e, HttpServletRequest request) {
        ExceptionBody exceptionBody = convertToExceptionBody(e);

        // 如果子类没有处理，则创建默认错误体
        if (exceptionBody == null) {
            exceptionBody = createDefaultExceptionBody(e);
        }

        exceptionBody.setException(e.getClass().getName());
        exceptionBody.setPath(request.getRequestURI());

        // 统一打印日志
        logger.error("Request [{}] failed: {}", request.getRequestURI(), exceptionBody.getMessage(), e);

        return ResponseEntity.status(exceptionBody.getHttpStatus()).body(exceptionBody);
    }

    /**
     * 子类可重写：将特定异常转成 ExceptionBody
     */
    protected ExceptionBody convertToExceptionBody(Exception srcException) {
        return null;
    }

    /**
     * 默认错误响应体：500 + 异常信息
     */
    private ExceptionBody createDefaultExceptionBody(Exception e) {
        HttpStatus status = HttpStatus.INTERNAL_SERVER_ERROR;
        String message = (e.getMessage() != null && !e.getMessage().isEmpty())
                ? e.getMessage()
                : status.getReasonPhrase();
        return new ExceptionBody(status, status.value(), status.getReasonPhrase(), message);
    }
}
