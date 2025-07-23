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

   Source File Name = ListNestedClassA.java

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

import java.util.LinkedList;
import java.util.List;

public class ListNestedClassA {
    private List<BasicClass> listEmpty = new LinkedList<BasicClass>();
    private List<BasicClass> listNull = null;
    private List<BasicClass> listHaveEles = new LinkedList<BasicClass>();

    public ListNestedClassA() {
        for (int i = 0; i < 10; i++) {
            listHaveEles.add(new BasicClass());
        }
    }

    public List<BasicClass> getListHaveEles() {
        return listHaveEles;
    }

    public void setListHaveEles(List<BasicClass> value) {
        listHaveEles = value;
    }

    public List<BasicClass> getListEmpty() {
        return listEmpty;
    }

    public void setListEmpty(List<BasicClass> value) {
        listEmpty = value;
    }

    public List<BasicClass> getListNull() {
        return listNull;
    }

    public void setListNull(List<BasicClass> value) {
        listNull = value;
    }

    @Override
    public boolean equals(Object otherObj) {
        if (!(otherObj instanceof ListNestedClassA)) {
            return false;
        }

        ListNestedClassA other = (ListNestedClassA) otherObj;

        if ((this.listHaveEles == null && other.listHaveEles != null)
            || (this.listHaveEles != null && !this.listHaveEles
            .equals(other.listHaveEles))) {
            return false;
        }

        if ((this.listEmpty == null && other.listEmpty != null)
            || (this.listEmpty != null && !this.listEmpty
            .equals(other.listEmpty))) {
            return false;
        }

        if ((this.listNull == null && other.listNull != null)
            || (this.listNull != null && !this.listNull
            .equals(other.listNull))) {
            return false;
        }

        return true;
    }
}