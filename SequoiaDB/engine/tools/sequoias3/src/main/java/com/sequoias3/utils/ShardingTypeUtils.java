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

   Source File Name = ShardingTypeUtils.java

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

import com.sequoias3.common.DataShardingType;

import java.text.SimpleDateFormat;
import java.util.Date;

public class ShardingTypeUtils {
    public static final String QUARTER_1      = "Q1";
    public static final String QUARTER_2      = "Q2";
    public static final String QUARTER_3      = "Q3";
    public static final String QUARTER_4      = "Q4";

    public static String getShardingTypeStr(DataShardingType shardingType, Date date){
        switch (shardingType){
            case YEAR:
                SimpleDateFormat yearFormat   = new SimpleDateFormat("yyyy");
                return yearFormat.format(date);
            case MONTH:
                SimpleDateFormat monthFormat = new SimpleDateFormat("MM");
                return monthFormat.format(date);
            case QUARTER:
                SimpleDateFormat quarterFormat = new SimpleDateFormat("MM");
                return getQuarter(Integer.parseInt(quarterFormat.format(date)));
            default:
                return null;
        }
    }

    private static String getQuarter(int month){
        switch (month){
            case 1:
            case 2:
            case 3:
                return QUARTER_1;
            case 4:
            case 5:
            case 6:
                return QUARTER_2;
            case 7:
            case 8:
            case 9:
                return QUARTER_3;
            case 10:
            case 11:
            case 12:
                return QUARTER_4;
            default:
                return null;
        }
    }
}
