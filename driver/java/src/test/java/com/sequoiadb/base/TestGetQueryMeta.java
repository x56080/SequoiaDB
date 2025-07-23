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

   Source File Name = TestGetQueryMeta.java

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
import org.junit.Test;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

public class TestGetQueryMeta extends SingleCSCLTestCase {
    @Test
    public void testGetQueryMeta() {
        BSONObject doc = new BasicBSONObject();
        doc.put("test", 100);
        cl.insert(doc);

        BSONObject matcher = new BasicBSONObject();
        matcher.put("test", 100);

        DBCursor cursor = cl.getQueryMeta(matcher, null, null, 0, -1, 0);
        assertTrue(cursor.hasNext());
        BSONObject obj = cursor.getNext();
        assertFalse(cursor.hasNext());
    }
}
