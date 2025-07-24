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

   Source File Name = RegionUtil.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoias3.util;

import com.sequoias3.model.CreateRegionRequest;

import java.util.regex.Pattern;

public class RegionUtil {

    public static void validateRegion(CreateRegionRequest request) throws IllegalArgumentException{
        if (null == request){
            throw new IllegalArgumentException("request cannot be null");
        }

        isValidRegionName(request.getRegion().getName());
    }

    public static void isValidRegionName(String regionName){
        if (null == regionName){
            throw new IllegalArgumentException("Region name cannot be null");
        }
        String pattern = "[a-zA-Z0-9-]{3,20}";
        if (!Pattern.matches(pattern, regionName)){
            throw new IllegalArgumentException("region name is invalid. regionName:"+regionName);
        }
    }
}
