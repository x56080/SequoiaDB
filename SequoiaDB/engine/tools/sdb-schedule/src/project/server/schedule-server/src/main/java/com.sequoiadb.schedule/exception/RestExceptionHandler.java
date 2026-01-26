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
   defect Date        Who Description
   ====== =========== === ==============================================
          26/01/2026  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.schedule.exception;

import org.springframework.http.HttpStatus;
import org.springframework.web.bind.annotation.ControllerAdvice;

@ControllerAdvice
public class RestExceptionHandler extends RestExceptionHandlerBase {

    @Override
    protected ExceptionBody convertToExceptionBody(Exception srcException) {
        if (!(srcException instanceof ScheduleServerException)) {
            return null;
        }

        ScheduleServerException e = (ScheduleServerException) srcException;
        HttpStatus status;
        if (e.getError() == ScheduleServerError.UNKNOWN_ERROR) {
            status = HttpStatus.BAD_REQUEST;
        }
        else {
            status = HttpStatus.INTERNAL_SERVER_ERROR;
        }

        return new ExceptionBody(status, status.value(), e.getError().getDesc(),
                ExceptionUtils.getExceptionMsgWithCauseBy(e));
    }
}
