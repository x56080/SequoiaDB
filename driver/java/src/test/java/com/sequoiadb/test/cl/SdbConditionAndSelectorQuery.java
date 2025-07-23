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

   Source File Name = SdbConditionAndSelectorQuery.java

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
import com.sequoiadb.test.common.ConstantsInsert;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.junit.*;

import java.util.List;

import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;


public class SdbConditionAndSelectorQuery {
    private static Sequoiadb sdb;
    private static CollectionSpace cs;
    private static DBCollection cl;

    @BeforeClass
    public static void setConnBeforeClass() throws Exception {
        // sdb
        sdb = new Sequoiadb(Constants.COOR_NODE_CONN, "", "");
        // cs
        if (sdb.isCollectionSpaceExist(Constants.TEST_CS_NAME_1)) {
            sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
            cs = sdb.createCollectionSpace(Constants.TEST_CS_NAME_1);
        } else {
            cs = sdb.createCollectionSpace(Constants.TEST_CS_NAME_1);
        }
        // cl
        BSONObject conf = new BasicBSONObject();
        conf.put("ReplSize", 0);
        cl = cs.createCollection(Constants.TEST_CL_NAME_1, conf);
//		List<BSONObject>list = ConstantsInsert.createRecordList(100);
//		cl.bulkInsert(list, 0);
    }

    @AfterClass
    public static void DropConnAfterClass() throws Exception {

        sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
        sdb.disconnect();
    }

    @Before
    public void setUp() throws Exception {
        List<BSONObject> list = ConstantsInsert.createRecordList(100);
        cl.bulkInsert(list, 0);
    }

    @After
    public void tearDown() throws Exception {
        cl.truncate();
    }

    @Test
//	@Ignore
    public void testSdbConditionAndSelectorQuery() {
        BSONObject query = new BasicBSONObject();
        BSONObject con = new BasicBSONObject();
        BSONObject sel = new BasicBSONObject();
        BSONObject orderBy = new BasicBSONObject();
        BSONObject hint = new BasicBSONObject();

        con.put("$gte", 0);
        con.put("$lte", 20);

        query.put("Id", con);

        sel.put("Id", "");
        sel.put("str", "");

        orderBy.put("Id", -1);

        hint.put("", "Id");

        DBCursor cursor = cl.query(query, sel, orderBy, hint);
        int i = 20;
        while (cursor.hasNext()) {
            if (!((cursor.getNext().get("Id").toString()).equals(Integer.toString(i)))) {
                assertTrue(false);
                break;
            }
            i--;
        }
    }

    @Test
    public void testGetCount() {
        BSONObject query = new BasicBSONObject();
        BSONObject con = new BasicBSONObject();
        con.put("$gte", 1);
        con.put("$lte", 10);

        query.put("Id", con);

        long count = cl.getCount(query);
        Assert.assertEquals(10, count);
        assertTrue(count == 10);
    }

    @Test
//	@Ignore
    public void testDelete() {
        BSONObject con = new BasicBSONObject();
        con.put("Id", 0);
        cl.delete(con);
        DBCursor cursor = cl.query(con, null, null, null);
        assertNull(cursor.getNext());
    }

    @Test
//	@Ignore
    public void testDeleteByHint() {
        BSONObject con = new BasicBSONObject();
        BSONObject hint = new BasicBSONObject();
        con.put("Id", 1);
        hint.put("", "Id");
        cl.delete(con, hint);
        DBCursor cursor = cl.query(con, null, null, hint);
        assertNull(cursor.getNext());
    }
}
