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

   Source File Name = ExceptionUtils.java

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

import java.net.SocketTimeoutException;
import java.util.LinkedHashSet;

public class ExceptionUtils {

    public static boolean causedBySocketTimeout(Throwable e) {
        if (e == null) {
            return false;
        }
        if (e instanceof SocketTimeoutException) {
            return true;
        }
        return causedBySocketTimeout(e.getCause());
    }

    public static String getExceptionMsgWithCauseBy(Throwable e) {
        StringBuilder exceptionMsg = new StringBuilder(e.getMessage());

        LinkedHashSet<String> existMessages = new LinkedHashSet<String>();
        existMessages.add(e.getMessage());

        while (true) {
            Throwable causeBy = e.getCause();
            if (causeBy == null) {
                break;
            }

            String message = causeBy.getMessage();
            if (existMessages.contains(message)) {
                break;
            }
            existMessages.add(message);
            exceptionMsg.append("\ncause by: ").append(message);
            if (!causeBy.getClass().getName().startsWith("com.sequoiacm")) {
                break;
            }
            e = causeBy;
        }
        return exceptionMsg.toString();
    }
}
