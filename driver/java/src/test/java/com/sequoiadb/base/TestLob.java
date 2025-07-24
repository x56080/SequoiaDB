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

   Source File Name = TestLob.java

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
import org.bson.types.ObjectId;
import org.junit.Test;

import java.io.UnsupportedEncodingException;

import static org.junit.Assert.*;

public class TestLob extends SingleCSCLTestCase {
    @Test
    public void testLob() {
        String str = "Hello, world!";

        ObjectId id = ObjectId.get();
        DBLob lob = cl.createLob(id);
        try {
            lob.write(str.getBytes("UTF-8"));
        } catch (UnsupportedEncodingException e) {
            fail(e.toString());
        }
        lob.close();

        DBCursor cursor = cl.listLobs();
        assertTrue(cursor.hasNext());
        BSONObject obj = cursor.getNext();
        ObjectId oid = (ObjectId) obj.get("Oid");
        assertEquals(id, oid);
        assertFalse(cursor.hasNext());

        lob = cl.openLob(id);
        byte[] bytes = new byte[(int) lob.getSize()];
        lob.read(bytes);
        try {
            String s = new String(bytes, "UTF-8");
            assertEquals(str, s);
        } catch (UnsupportedEncodingException e) {
            fail(e.toString());
        }
        lob.close();

        cl.removeLob(id);
        cursor = cl.listLobs();
        assertFalse(cursor.hasNext());
    }
}
