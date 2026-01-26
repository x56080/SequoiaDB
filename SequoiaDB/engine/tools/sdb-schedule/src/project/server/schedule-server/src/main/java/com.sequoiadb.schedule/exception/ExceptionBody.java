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

   Source File Name = ExceptionBody.java

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

import org.bson.BasicBSONObject;
import org.springframework.http.HttpStatus;

import com.fasterxml.jackson.annotation.JsonIgnore;

public class ExceptionBody {
    private long timestamp;
    private int status;
    private String error;
    private String exception;
    private String message;
    private String path;

    @JsonIgnore
    private boolean needSessionId = true;

    @JsonIgnore
    private HttpStatus httpStatus;

    public ExceptionBody(HttpStatus httpStatus, int errorCode, String errorCodeDesc, String message,
            boolean needSessionId) {
        this.httpStatus = httpStatus;
        this.timestamp = System.currentTimeMillis();
        this.status = errorCode;
        this.error = errorCodeDesc;
        this.message = message;
        this.needSessionId = needSessionId;
    }

    public ExceptionBody(HttpStatus httpStatus, int errorCode, String errorCodeDesc,
            String message) {
        this(httpStatus, errorCode, errorCodeDesc, message, true);
    }

    public ExceptionBody(int errorCode, String errorCodeDesc, String message) {
        this(HttpStatus.INTERNAL_SERVER_ERROR, errorCode, errorCodeDesc, message);
    }

    public ExceptionBody(int errorCode, String errorCodeDesc, String message,
            boolean needSessionId) {
        this(HttpStatus.INTERNAL_SERVER_ERROR, errorCode, errorCodeDesc, message, needSessionId);
    }

    public long getTimestamp() {
        return timestamp;
    }

    public int getStatus() {
        return status;
    }

    public String getError() {
        return error;
    }

    public String getException() {
        return exception;
    }

    public String getMessage() {
        return message;
    }

    void setMessage(String message) {
        this.message = message;
    }

    public String getPath() {
        return path;
    }

    void setPath(String path) {
        this.path = path;
    }

    void setException(String exception) {
        this.exception = exception;
    }

    public HttpStatus getHttpStatus() {
        return httpStatus;
    }

    public boolean isNeedSessionId() {
        return needSessionId;
    }

    void setNeedSessionId(boolean needSessionId) {
        this.needSessionId = needSessionId;
    }

    public String toJson() {
        BasicBSONObject object = new BasicBSONObject();
        object.put("timestamp", timestamp);
        object.put("status", status);
        object.put("error", error);
        object.put("exception", exception);
        object.put("message", message);
        object.put("path", path);
    
        return object.toString();
    }
}
