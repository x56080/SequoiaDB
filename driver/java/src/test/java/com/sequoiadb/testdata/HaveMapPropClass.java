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

   Source File Name = HaveMapPropClass.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.testdata;

import java.util.Arrays;
import java.util.HashMap;
import java.util.Map;

public class HaveMapPropClass {
    private Map<String, String> mapProp = null;
    private Map<String, User> userMap = null;

    public HaveMapPropClass() {
        mapProp = new HashMap<String, String>();
    }

    public Map<String, String> getMapProp() {
        return mapProp;
    }

    public void setMapProp(Map<String, String> value) {
        this.mapProp = value;
    }

    public Map<String, User> getUserMap() {
        return this.userMap;
    }

    public void setUserMap(Map<String, User> userMap) {
        this.userMap = userMap;
    }

    @Override
    public String toString() {
        return "HaveMapPropClass [mapProp=" + mapProp + ", userMap=" + userMap
            + "]";
    }

    @Override
    public int hashCode() {
        Object[] objects = new Object[]{mapProp, userMap};
        return Arrays.hashCode(objects);
    }

    @Override
    public boolean equals(Object obj) {
        if (obj instanceof HaveMapPropClass) {
            if (this == obj) {
                return true;
            }

            HaveMapPropClass other = (HaveMapPropClass) obj;
            return this.mapProp.equals(other.mapProp) && this.userMap.equals(other.userMap);
        } else {
            return false;
        }
    }
}
