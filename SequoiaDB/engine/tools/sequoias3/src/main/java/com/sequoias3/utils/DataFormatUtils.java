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

   Source File Name = DataFormatUtils.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoias3.utils;

import com.sequoias3.exception.S3Error;
import com.sequoias3.exception.S3ServerException;

import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;
import java.util.TimeZone;

public class DataFormatUtils {
    public static final String DATA_PATTERN = "yyyy-MM-dd'T'HH:mm:ss.SSS'Z'";

    public static String formatDate(long time){
        SimpleDateFormat sdf = new SimpleDateFormat(DATA_PATTERN);
        sdf.setTimeZone(TimeZone.getTimeZone("UTC"));
        return sdf.format(new Date(time));
    }

    public static String formateDate2(long time){
        SimpleDateFormat sdf = new SimpleDateFormat("EEE, dd MMM yyyy HH:mm:ss 'GMT'", Locale.ENGLISH);
        return sdf.format(new Date(time));
    }

    public static Date parseDate(String dateString) throws S3ServerException {
        try {
            SimpleDateFormat simpleDateFormat = new SimpleDateFormat("EEE, dd MMM yyyy HH:mm:ss ZZZ", Locale.ENGLISH);
            return simpleDateFormat.parse(dateString);
        } catch(ParseException e){
            throw new S3ServerException(S3Error.OBJECT_INVALID_TIME, "dateString is invalid. dateString:"+dateString, e);
        }
    }

    public static Date parseXAMZDate(String dateString) throws S3ServerException {
        try {
            SimpleDateFormat simpleDateFormat = new SimpleDateFormat("yyyyMMdd'T'HHmmss'Z'", Locale.ENGLISH);
            simpleDateFormat.setTimeZone(TimeZone.getTimeZone("UTC"));
            return simpleDateFormat.parse(dateString);
        } catch(ParseException e){
            throw new S3ServerException(S3Error.OBJECT_INVALID_TIME, "dateString is invalid. dateString:"+dateString, e);
        }
    }
}
