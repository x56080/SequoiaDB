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

   Source File Name = TestCreateIdIndex.java

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
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.test.common.Constants;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.junit.*;

import static org.junit.Assert.assertEquals;

public class TestCreateIdIndex {
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
    public void testDelete() {
        String strAge = "age";
        int iAge = 10;
        boolean canDelete = true;
        cl.insert(new BasicBSONObject(strAge, iAge));

        // once dropIdIndex, delete can't be execute;
        cl.dropIdIndex();

        try {
            cl.delete(new BasicBSONObject(strAge, iAge));
            canDelete = true;
        } catch (BaseException e) {
            canDelete = false;
        }

        assertEquals(canDelete, false);

        // once createIdIndex, delete can be execute
        cl.createIdIndex(new BasicBSONObject());
        try {
            cl.delete(new BasicBSONObject(strAge, iAge));
            canDelete = true;
        } catch (BaseException e) {
            canDelete = false;
        }
        assertEquals(canDelete, true);
    }

    @Test
    public void testUpdate() {
        String strAge = "age";
        int iAge = 10;
        int iAgeNew = 11;
        boolean canUpdate = true;
        cl.insert(new BasicBSONObject(strAge, iAge));

        BSONObject modifier = new BasicBSONObject();
        modifier.put("$set", new BasicBSONObject(strAge, iAgeNew));

        // once dropIdIndex, delete can't be execute;
        cl.dropIdIndex();

        try {
            cl.update(new BasicBSONObject(strAge, iAge), modifier, null);
            ;
            canUpdate = true;
        } catch (BaseException e) {
            canUpdate = false;
        }

        assertEquals(canUpdate, false);

        // once createIdIndex, delete can be execute
        cl.createIdIndex(new BasicBSONObject());
        try {
            cl.update(new BasicBSONObject(strAge, iAge), modifier, null);
            ;
            canUpdate = true;
        } catch (BaseException e) {
            canUpdate = false;
        }
        assertEquals(canUpdate, true);
    }
}