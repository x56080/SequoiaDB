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

   Source File Name = CollectionSchemaTest.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.test.schema;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.test.common.Constants;
import org.bson.BasicBSONObject;
import org.junit.*;

public class CollectionSchemaTest {

    private static Sequoiadb sdb;
    private static String csName = "schemaCS";
    private static String clName = "schemaCL";
    private static String schemaName = "schema_01";
    private static CollectionSpace cs;
    private static DBCollection cl;

    @BeforeClass
    public static void init() {
        sdb = new Sequoiadb(Constants.COOR_NODE_CONN, "", "");
    }

    @Test
    public void test() {
        addSchema();
        alterCollection();
        enableSchema();
    }

    public void createSchema() {
        BasicBSONObject col = new BasicBSONObject();
        BasicBSONObject type = new BasicBSONObject();
        type.put("Type", "string");
        type.put("ReadDefault", "aa");
        type.put("WriteDefault", "bb");
        col.put("field", type);
        sdb.createSchema(schemaName, col);
    }

    public void addSchema() {
        createSchema();
        BasicBSONObject opt = new BasicBSONObject();
        opt.put("EnableInfoSchema", true);
        cs = sdb.createCollectionSpace(csName);
        cl = cs.createCollection(clName, opt);
        cl.addSchema(schemaName);
        sdb.dropCollectionSpace(csName);
    }

    public void alterCollection() {
        createSchema();
        cl = sdb.createCollectionSpace(csName).createCollection(clName);
        BasicBSONObject opt = new BasicBSONObject();
        opt.put("EnableInfoSchema", true);
        try {
            cl.addSchema(schemaName);
            Assert.fail();
        } catch (BaseException e) {
            Assert.assertEquals(e.getErrorCode(), SDBError.SDB_OPERATION_INCOMPATIBLE.getErrorCode());
        }
        cl.alterCollection(opt);
        cl.addSchema(schemaName);
        sdb.dropCollectionSpace(csName);
    }

    public void enableSchema() {
        createSchema();
        cl = sdb.createCollectionSpace(csName).createCollection(clName);
        try {
            cl.addSchema(schemaName);
            Assert.fail();
        } catch (BaseException e) {
            Assert.assertEquals(e.getErrorCode(), SDBError.SDB_OPERATION_INCOMPATIBLE.getErrorCode());
        }
        cl.enableInfoSchema();
        cl.addSchema(schemaName);
        sdb.dropCollectionSpace(csName);
    }
}
