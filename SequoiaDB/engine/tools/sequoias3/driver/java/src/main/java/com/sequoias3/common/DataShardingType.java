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

   Source File Name = DataShardingType.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoias3.common;

public enum DataShardingType {
    /**
     * year
     */
    YEAR("year"),

    /**
     * month
     */
    MONTH("month"),

    /**
     * quarter
     */
    QUARTER("quarter");

    private String desc;

    DataShardingType(String name) {
        this.desc = name;
    }

    public String getDesc(){
        return desc;
    }

    @Override
    public String toString() {
        return desc;
    }

    public static DataShardingType getType(String name){
        for (DataShardingType type : DataShardingType.values()){
            if (type.getDesc().equals(name)){
                return type;
            }
        }
        return null;
    }
}
