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

   Source File Name = MultiThreadCreateGetDropCS.java

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

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;

public class MultiThreadCreateGetDropCS implements Runnable {
    Sequoiadb sdb;
    CollectionSpace cs;
    DBCollection cl;

    public MultiThreadCreateGetDropCS() {
        sdb = new Sequoiadb(Constants.COOR_NODE_CONN, "", "");
    }

    @Override
    public void run() {
        try {
            sdb.createCollectionSpace(Constants.TEST_CS_NAME_1);
        } catch (BaseException e) {
            System.out.println(this.hashCode() + " " + e.getMessage());
        }
        try {
            sdb.getCollectionSpace(Constants.TEST_CS_NAME_1);
        } catch (BaseException e) {
            System.out.println(this.hashCode() + " " + e.getMessage());
        }
        try {
            sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
        } catch (BaseException e) {
            System.out.println(this.hashCode() + " " + e.getMessage());
        }
    }
}