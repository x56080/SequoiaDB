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

   Source File Name = NestedClassA.java

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

public class NestedClassA {
    private BasicClass basicObjA = new BasicClass();
    private BasicClass basicObjB = new BasicClass();
    private BasicClass basicObjC = null;

    public BasicClass getBasicObjA() {
        return basicObjA;
    }

    public void setBasicObjA(BasicClass value) {
        basicObjA = value;
    }

    public BasicClass getBasicObjB() {
        return basicObjB;
    }

    public void setBasicObjB(BasicClass value) {
        basicObjB = value;
    }

    public BasicClass getBasicObjC() {
        return basicObjC;
    }

    public void setBasicObjC(BasicClass value) {
        basicObjC = value;
    }

    @Override
    public boolean equals(Object otherObj) {
        if (otherObj == null || !(otherObj instanceof NestedClassA)) {
            return false;
        }

        NestedClassA other = (NestedClassA) otherObj;
        if ((this.basicObjA == null && other.basicObjA != null) ||
            (this.basicObjA != null && !this.basicObjA.equals(other.basicObjA))) {
            return false;
        }

        if ((this.basicObjB == null && other.basicObjB != null) ||
            (this.basicObjB != null && !this.basicObjB.equals(other.basicObjB))) {
            return false;
        }

        if ((this.basicObjC == null && other.basicObjC != null) ||
            (this.basicObjC != null && !this.basicObjC.equals(other.basicObjC))) {
            return false;
        }

        return true;

    }
}