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

   Source File Name = testMutilThreadUpdate.java

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
import com.sequoiadb.test.common.MultiThreadUpdate;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.junit.*;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;


public class testMutilThreadUpdate {
    private static Sequoiadb sdb;
    private static CollectionSpace cs;
    private static DBCollection cl;
    private static DBCursor cursor;
    static final int num = 10;


    @BeforeClass
    public static void setConnBeforeClass() throws Exception {
        // sdb
        sdb = new Sequoiadb(Constants.COOR_NODE_CONN, "", "");
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
        long recordNum = cl.getCount();
        assertTrue((num * THREAD_COUNT) == recordNum);
    }

    @AfterClass
    public static void DropConnAfterClass() throws Exception {
        sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
        sdb.disconnect();
    }

    @Before
    public void setUp() throws Exception {
    }

    @After
    public void tearDown() throws Exception {
    }

    @Test
    public void testThreadUpdate() {
        final int THREAD_COUNT = 10;
        Thread[] updateThreadList = new Thread[THREAD_COUNT];
        for (int i = 0; i < THREAD_COUNT; i++) {
            updateThreadList[i] = new Thread(new MultiThreadUpdate());
            updateThreadList[i].start();
        }
        for (int i = 0; i < THREAD_COUNT; i++) {
            try {
                updateThreadList[i].join();
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
        for (int i = 0; i < THREAD_COUNT; i++) {
            System.out.println("i is " + i);
            BSONObject query = new BasicBSONObject();
            String field = "field" + updateThreadList[i].getId();
            query.put(field, updateThreadList[i].getName());

            int size = 0;
            //System.out.println(query);
            cursor = cl.query(query, null, null, null);
            if (cursor == null)
                System.out.println("cursor is null");
            while (cursor.hasNext()) {
                cursor.getNext();
                size++;
            }
            System.out.println("size is " + size);
            assertEquals(size, 100);
        }
    }
}
