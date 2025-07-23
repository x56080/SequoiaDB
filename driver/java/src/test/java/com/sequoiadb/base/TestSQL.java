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

   Source File Name = TestSQL.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.base;

import com.sequoiadb.test.SingleCSCLTestCase;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;
import org.junit.Test;

import static org.junit.Assert.*;

public class TestSQL extends SingleCSCLTestCase {
    @Override
    public void setUp() {
        super.setUp();
        cl.truncate();
    }

    @Override
    public void tearDown() {
        super.tearDown();
        cl.truncate();
    }

    @Test
    public void testExec() {
        BSONObject obj = new BasicBSONObject();
        obj.put("test", ObjectId.get());

        cl.insert(obj);

        String sql = String.format("select * from %s", cl.getFullName());
        DBCursor cursor = sdb.exec(sql);
        assertTrue(cursor.hasNext());
        BSONObject retObj = cursor.getNext();
        assertEquals(obj, retObj);
        assertFalse(cursor.hasNext());

        sql = String.format("select * from %s where canm <> '' and canm = '清凉溪' limit 3", cl.getFullName());
        cursor = sdb.exec(sql);
    }

    @Test
    public void testExecUpdate() {
        ObjectId oid = ObjectId.get();
        ObjectId newOid = ObjectId.get();

        String name = "test";

        BSONObject obj = new BasicBSONObject();
        obj.put(name, oid.toString());
        cl.insert(obj);

        obj.removeField(name);
        obj.put(name, newOid.toString());

        String sql = String.format("update %s set test='%s' where test='%s'",
            cl.getFullName(), newOid.toString(), oid.toString());
        sdb.execUpdate(sql);

        sql = String.format("select * from %s", cl.getFullName());
        DBCursor cursor = sdb.exec(sql);
        assertTrue(cursor.hasNext());
        BSONObject retObj = cursor.getNext();
        assertEquals(obj, retObj);
        assertFalse(cursor.hasNext());
    }
}
