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

   Source File Name = BSONCodeTypeTest.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.test.bson;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.test.common.Constants;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.junit.*;

public class BSONCodeTypeTest {

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
        }
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
    public void code_type_display_test() {
        if (!Constants.isCluster()) {
            return;
        }
        String name = "abc_in_java";
        String code = "function abc_in_java(x, y){return x + y ;}";
        sdb.crtJSProcedure(code);
        BSONObject obj = null;
        try {
            cursor = sdb.listProcedures(new BasicBSONObject().append("name", name));
            try {
                while (cursor.hasNext()) {
                    obj = cursor.getNext();
                }
            } finally {
                cursor.close();
            }
            // check
            Assert.assertTrue(obj != null);
            obj.removeField("_id");
            System.out.println("obj is: " + obj);
            String retStr = "{ \"name\" : \"abc_in_java\" , \"func\" : { \"$code\" : \"function abc_in_java(x, y){return x + y ;}\" } , \"funcType\" : 0 }";
            Assert.assertEquals(retStr, obj.toString());
        } finally {
            try {
                sdb.rmProcedure(name);
            } catch (BaseException e) {
                System.out.println("Failed to remove js procedure");
                System.out.println("Error message is: " + e.getMessage() + e.getErrorCode());
            }
        }
    }

}
