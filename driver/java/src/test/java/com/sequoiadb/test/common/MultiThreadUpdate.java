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

   Source File Name = MultiThreadUpdate.java

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
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;

public class MultiThreadUpdate implements Runnable {
    Sequoiadb sdb;
    CollectionSpace cs;
    DBCollection cl;
    DBCursor cursor;

    public MultiThreadUpdate() {
        sdb = new Sequoiadb(Constants.COOR_NODE_CONN, "", "");
        if (sdb.isCollectionSpaceExist(Constants.TEST_CS_NAME_1)) {
            cs = sdb.getCollectionSpace(Constants.TEST_CS_NAME_1);
            if (cs.isCollectionExist(Constants.TEST_CL_NAME_1))
                cl = cs.getCollection(Constants.TEST_CL_NAME_1);
            else
                cl = cs.createCollection(Constants.TEST_CL_NAME_1);
        } else {
            cs = sdb.createCollectionSpace(Constants.TEST_CS_NAME_1);
            cl = cs.createCollection(Constants.TEST_CL_NAME_1);
        }
    }

    @Override
    public void run() {
        //System.out.println("Update�߳�==="+Thread.currentThread().getId()+"ִ�п�ʼ");
        BSONObject modifier = new BasicBSONObject();
        BSONObject m = new BasicBSONObject();
        modifier.put("$set", m);
        String field = "field" + Thread.currentThread().getId();
        m.put(field, Thread.currentThread().getName());

        cl.update(null, modifier, null);
        //System.out.println("Update�߳�==="+Thread.currentThread().getId()+"ִ�н���");
    }
}