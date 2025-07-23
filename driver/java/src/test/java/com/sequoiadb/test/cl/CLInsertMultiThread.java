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

   Source File Name = CLInsertMultiThread.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.test.cl;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.test.common.Constants;
import com.sequoiadb.test.common.MultiThreadInsert;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.junit.*;

import static org.junit.Assert.assertEquals;

public class CLInsertMultiThread {
    private static Sequoiadb sdb;
    private static CollectionSpace cs;
    private static DBCollection cl;
    private static DBCursor cursor;


    @BeforeClass
    public static void setConnBeforeClass() throws Exception {
        // sdb
        sdb = new Sequoiadb(Constants.COOR_NODE_CONN, "", "");
    }

    @AfterClass
    public static void DropConnAfterClass() throws Exception {
        sdb.disconnect();
    }

    @Before
    public void setUp() throws Exception {
        // cs
        if (sdb.isCollectionSpaceExist(Constants.TEST_CS_NAME_1)) {
            sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
            cs = sdb.createCollectionSpace(Constants.TEST_CS_NAME_1);
        } else
            cs = sdb.createCollectionSpace(Constants.TEST_CS_NAME_1);
        // cl
        BSONObject conf = new BasicBSONObject();
        conf.put("ReplSize", 0);
        cl = cs.createCollection(Constants.TEST_CL_NAME_1, conf);
    }

    @After
    public void tearDown() throws Exception {
        sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
    }

    @Test
    public void testThreadInsert() {
        final int THREAD_COUNT = 10;
        Thread[] insertThreadList = new Thread[THREAD_COUNT];
        for (int i = 0; i < THREAD_COUNT; i++) {
            insertThreadList[i] = new Thread(new MultiThreadInsert());
            insertThreadList[i].start();
        }
        for (int i = 0; i < THREAD_COUNT; i++) {
            try {
                insertThreadList[i].join();
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }

        cursor = cl.query();
        int count = 0;
        while (cursor.hasNext()) {
            count++;
            cursor.getNext();
        }
        assertEquals(count, 100);
        for (int i = 0; i < THREAD_COUNT; i++) {
            for (int j = 0; j < 10; j++) {
                BSONObject query = new BasicBSONObject();

                query.put("ThreadID", insertThreadList[i].getId());
                query.put("NO", insertThreadList[i].getId() + "_" + String.valueOf(j));

                int size = 0;
                //System.out.println(query);
                cursor = cl.query(query, null, null, null);
                while (cursor.hasNext()) {
                    cursor.getNext();
                    size++;
                }
                assertEquals(size, 1);
            }
        }
    }
}
