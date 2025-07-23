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

   Source File Name = SdbNest.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.test.common;

public class SdbNest {

    public static String createNestElement(int num) {
        String str = "";
        int i = 0;
        if (i != num) {
            i++;
            str += ("{element:" + createNestElement(num - 1) + "}");
        } else
            return str += "{element:100}";
        return str;
    }

    public static String createNestArray(int num) {
        String arr = "";
        int j = 0;
        if (j != num) {
            j++;
            arr += ("{arr:[" + createNestArray(num - 1) + "]}");
        } else
            return arr += "{arr:[10,20,20]}";
        return arr;
    }

	/*public static void main(String[] args){
        SdbNest nest = new SdbNest();
	    String string2= nest.createNestElement(4);
	    System.out.println(string2);
		String string = nest.createNestArray(4);
		System.out.println(string);
	}*/
}
