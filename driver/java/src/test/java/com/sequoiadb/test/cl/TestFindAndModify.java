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

   Source File Name = TestFindAndModify.java

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
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.junit.*;

import static org.junit.Assert.assertEquals;

public class TestFindAndModify {
    private static Sequoiadb sdb;
    private static CollectionSpace cs;
    private static DBCollection cl;

    @BeforeClass
    public static void setConnBeforeClass() throws Exception {

    }

    @AfterClass
    public static void DropConnAfterClass() throws Exception {

    }

    @Before
    public void setUp() throws Exception {
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
    }

    @After
    public void tearDown() throws Exception {
        sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
        sdb.disconnect();
    }

    @Test
    public void testFindAndRemove() {
        BSONObject v1 = new BasicBSONObject();
        BSONObject v2 = new BasicBSONObject();
        v1.put("a", 1);
        v2.put("a", 2);
        cl.insert(v1);
        cl.insert(v2);


        DBCursor cursor = cl.queryAndRemove(v1, null, null, null, 0, -1, 0);
        while (cursor.hasNext()) {
            cursor.getNext();
        }

        cursor = cl.query();
        int i = 0;
        while (cursor.hasNext()) {
            BSONObject result = cursor.getNext();
            assertEquals(v2, result);
            i++;
        }
        assertEquals(1, i);
    }

    @Test
    public void testFindAndUpdate() {
        BSONObject v1 = new BasicBSONObject();
        BSONObject v2 = new BasicBSONObject();
        v1.put("a", 1);
        v1.put("b", 1);
        v2.put("a", 2);
        v2.put("b", 2);
        cl.insert(v1);
        cl.insert(v2);

        BSONObject selector = new BasicBSONObject("b", 1);
        BSONObject tmp = new BasicBSONObject("b", 1111);
        BSONObject update = new BasicBSONObject("$set", tmp);
        DBCursor cursor = cl.queryAndUpdate(v1, selector, null, null, update,
            0, -1, 0, true);
        while (cursor.hasNext()) {
            BSONObject result = cursor.getNext();
            System.out.println(result);
            assertEquals(true, result.equals(tmp));
        }

        cursor = cl.query(new BasicBSONObject("a", 1), null, null, null);
        int i = 0;
        while (cursor.hasNext()) {
            BSONObject result = cursor.getNext();
            System.out.println(result);
            int b = (Integer) result.get("b");
            assertEquals(1111, b);
            i++;
        }

        assertEquals(i, 1);
    }
}