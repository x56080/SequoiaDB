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

   Source File Name = Range.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoias3.core;

import com.sequoias3.common.RestParamDefine;
import com.sequoias3.exception.S3Error;
import com.sequoias3.exception.S3ServerException;

public class Range {
    private long start = -1;
    private long end   = -1;
    private long contentLength;

    public Range(String range) throws S3ServerException {
        int beginIndex = range.indexOf(RestParamDefine.REST_RANGE_START);
        if (-1 == beginIndex){
            throw new S3ServerException(S3Error.OBJECT_INVALID_RANGE, "range is invalid. range:"+range);
        }
        String substring = range.substring(beginIndex+RestParamDefine.REST_RANGE_START.length(), range.length());

        int firstHyphen = substring.indexOf(RestParamDefine.REST_HYPHEN);
        if (firstHyphen == -1){
            throw new S3ServerException(S3Error.OBJECT_INVALID_RANGE, "range  is invalid. range:"+range);
        }
        if (substring.indexOf(RestParamDefine.REST_HYPHEN, firstHyphen + 1) != -1){
            throw new S3ServerException(S3Error.OBJECT_INVALID_RANGE, "range  is invalid. range:"+range);
        }

        String[] numbers = substring.split(RestParamDefine.REST_HYPHEN);
        if (numbers.length == 2){
            try {
                if (numbers[0].length() > 0){
                    this.start = Long.parseLong(numbers[0]);
                }
                if (numbers[1].length() > 0){
                    this.end = Long.parseLong(numbers[1]);
                }
            }catch (NumberFormatException e){
                throw new S3ServerException(S3Error.OBJECT_INVALID_RANGE, "range is invalid"+range);
            }

            if (this.start > this.end){
                throw new S3ServerException(S3Error.OBJECT_INVALID_RANGE, "range is invalid"+range);
            }
        } else if (numbers.length == 1){
            if (numbers[0].length() > 0) {
                try {
                    if (substring.endsWith(RestParamDefine.REST_HYPHEN)) {
                        this.start = Long.parseLong(numbers[0]);
                    }
                    if (substring.startsWith(RestParamDefine.REST_HYPHEN)) {
                        this.end = Long.parseLong(numbers[0]);
                    }
                }catch (NumberFormatException e){
                    throw new S3ServerException(S3Error.OBJECT_INVALID_RANGE, "range is invalid"+range);
                }
            }
        }else {
            throw new S3ServerException(S3Error.OBJECT_INVALID_RANGE, "range is invalid"+range);
        }

        if (-1 == this.start && -1 == this.end){
            throw new S3ServerException(S3Error.OBJECT_INVALID_RANGE, "range is invalid"+range);
        }

    }

    public void setStart(long start) {
        this.start = start;
    }

    public long getStart() {
        return start;
    }

    public void setEnd(long end) {
        this.end = end;
    }

    public long getEnd() {
        return end;
    }

    public void setContentLength(long contentLength) {
        this.contentLength = contentLength;
    }

    public long getContentLength() {
        return contentLength;
    }
}
