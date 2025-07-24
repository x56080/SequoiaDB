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

   Source File Name = TestCollectionSpace.java

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

import com.sequoiadb.test.SingleTestCase;
import org.bson.types.ObjectId;
import org.junit.Test;

import static org.junit.Assert.*;

public class TestCollectionSpace extends SingleTestCase {
    private String csName;

    public void setUp() {
        csName = "TestCollectionSpace_" + new ObjectId().toString();
    }

    public void tearDown() {
        csName = null;
    }

    @Test
    public void testCreateDrop() {
        assertFalse(sdb.isCollectionSpaceExist(csName));

        CollectionSpace cs = sdb.createCollectionSpace(csName);
        assertEquals(csName, cs.getName());
        assertEquals(sdb, cs.getSequoiadb());
        assertEquals(0, cs.getCollectionNames().size());

        assertTrue(sdb.isCollectionSpaceExist(csName));
        sdb.dropCollectionSpace(csName);
        assertFalse(sdb.isCollectionSpaceExist(csName));
    }
}
