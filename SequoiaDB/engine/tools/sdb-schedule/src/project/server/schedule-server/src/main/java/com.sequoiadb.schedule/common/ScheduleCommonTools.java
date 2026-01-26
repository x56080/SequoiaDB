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

   Source File Name = ScheduleCommonTools.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/01/2026  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.schedule.common;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.Closeable;
import java.util.Date;

public class ScheduleCommonTools {
    private static final Logger logger = LoggerFactory.getLogger(ScheduleCommonTools.class);

    public static void exitProcess() {
        System.exit(-1);
    }
    /**
     * getDuration between end and begin
     *
     * @param begin
     *            begin date.
     * @param end
     *            end date.
     * @return the duration(seconds)
     */
    public static int getDuration(Date begin, Date end) {
        long l = begin.getTime();
        long e = end.getTime();
        if (e > l) {
            return (int) ((e - l) / 1000);
        }
        else {
            return (int) ((l - e) / 1000);
        }
    }

    public static String bytesToHexStr(byte[] b) {
        StringBuilder buf = new StringBuilder();
        for (byte value : b) {
            int x = value & 0xFF;
            String s = Integer.toHexString(x).toUpperCase();
            if (s.length() == 1) {
                buf.append("0");
            }

            buf.append(s);
        }

        return buf.toString();
    }

    public static void closeResource(Closeable... closeables) {
        if (closeables == null) {
            return;
        }
        for (Closeable closeable : closeables) {
            if (closeable != null) {
                try {
                    closeable.close();
                }
                catch (Exception e) {
                    logger.warn("Failed to close resource:{}", closeable, e);
                }

            }

        }
    }
}
