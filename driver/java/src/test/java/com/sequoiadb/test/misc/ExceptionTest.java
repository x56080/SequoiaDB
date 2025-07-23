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

   Source File Name = ExceptionTest.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.test.misc;

import com.sequoiadb.base.*;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.test.common.Constants;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.junit.*;


public class ExceptionTest {

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
    public void TestEquals() {
        Assert.assertEquals(true, SDBError.SDB_INVALIDARG.equals(SDBError.SDB_INVALIDARG));
        Assert.assertEquals(false, SDBError.SDB_SYS.equals(SDBError.SDB_INVALIDARG));
    }

    @Test
//	@Ignore
    public void TestDeprecatedExceptionAPI() {
        // case 1: test error type
        try {
            ConfigOptions opt = new ConfigOptions();
            opt.setMaxAutoConnectRetryTime(0);
            opt.setConnectTimeout(100);
            Sequoiadb db = new Sequoiadb("123:1234", "", "", opt);
            Assert.fail();
        } catch (BaseException e) {
            System.out.println("case 1's result:");
            System.out.println("error type: " + e.getErrorType());
            System.out.println("error code: " + e.getErrorCode());
            System.out.println("error desc: " + e.getMessage());
            System.out.println("error stack: ");
            e.printStackTrace();
            Assert.assertEquals(SDBError.SDB_NETWORK.getErrorType(), e.getErrorType());
            Assert.assertEquals(SDBError.SDB_NETWORK.getErrorCode(), e.getErrorCode());
            Assert.assertEquals("SDB_NETWORK(-15): Network error, detail: failed to connect to /0.0.0.123:1234", e.getMessage());
        }

		// case 2: test error code
		try {
            cs.dropCollection("cs_not_exist");
			Assert.fail();
		} catch(BaseException e) {
			System.out.println("case 2's result:");
			System.out.println("error type: " + e.getErrorType());
			System.out.println("error code: " + e.getErrorCode());
			System.out.println("error desc: " + e.getMessage());
			System.out.println("error stack: ");
			e.printStackTrace();
		}

    }

    @Test
//	@Ignore
    public void TestFormalExceptionAPI() {
        try {
            try {
                ConfigOptions opt = new ConfigOptions();
                opt.setMaxAutoConnectRetryTime(0);
                opt.setConnectTimeout(100);
                Sequoiadb db = new Sequoiadb("123", "", "", opt);
                Assert.fail();
            } catch (BaseException e) {
                throw new BaseException(SDBError.SDB_SYS, "this is a test message", e);
            }
        } catch (BaseException e) {
            System.out.println("result of TestFormalExceptionAPI is:");
            System.out.println("error type: " + e.getErrorType());
            System.out.println("error code: " + e.getErrorCode());
            System.out.println("error desc: " + e.getMessage());
            System.out.println("error stack: ");
            e.printStackTrace();
            Assert.assertEquals(SDBError.SDB_SYS.getErrorType(), e.getErrorType());
            Assert.assertEquals(SDBError.SDB_SYS.getErrorCode(), e.getErrorCode());
            Assert.assertEquals("SDB_SYS(-10): System error, detail: this is a test message", e.getMessage());
        }

    }

    @Test
    public void testExceptionInfo() {

        try {
            BSONObject obj = new BasicBSONObject();
            obj.put("_id", 1);
            cl.insert(obj);
            cl.insert(obj);
        } catch (BaseException e) {
            e.printStackTrace();
        }
    }

}
