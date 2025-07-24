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

   Source File Name = Bug_JIRA_.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.test.bug;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.test.common.Constants;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSONParseException;
import org.junit.*;
import org.junit.rules.ExpectedException;

import java.util.Random;


public class Bug_JIRA_ {
    private static Sequoiadb sdb;
    private static CollectionSpace cs;
    private static DBCollection cl;
    @Rule
    public ExpectedException thrown = ExpectedException.none();

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
        cl.delete("");
    }

    @Test
    public void tmp() {
        String str = (String)null;
        if (str == null) {
            System.out.println("yes");
        }
    }

    @Test
    public void jira2163_insert_invalid_binary() {
        thrown.expect(JSONParseException.class);
        cl.insert("{ a: { '$binary': 'd29ybGQ', '$type': '1' } } ");
    }

    @Test
    public void jira_4923() {
        Sequoiadb mydb = new Sequoiadb(Constants.COOR_NODE_CONN, "", "");
//        DBCollection mycl =
//            mydb.getCollectionSpace("maincs").getCollection("maincl");
//            mydb.getCollectionSpace("mytest").getCollection("mytest");
//        long runTimes = 100000000L;
        long runTimes = 1L;
        int range = 1000;
        Random random = new Random();
        while(runTimes-- > 0) {
            BSONObject cond = new BasicBSONObject("a", random.nextInt(range));
            try {
                BSONObject obj = cl.queryOne(cond, null, null, null, -1);
//                System.out.println("obj is: " + obj.toString());
            } catch (BaseException e) {
                Assert.assertEquals(SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode());
            }
        }
    }


}
