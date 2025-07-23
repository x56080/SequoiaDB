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

   Source File Name = TestAggregate.java

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

import java.util.ArrayList;
import java.util.List;
import java.util.Random;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

public class TestAggregate extends SingleCSCLTestCase {
    @Test
    public void testAggregate() {
        final int n = 100;
        List<BSONObject> objs = new ArrayList<BSONObject>(n);
        Random rand = new Random();

        for (int i = 0; i < n; i++) {
            BSONObject obj = new BasicBSONObject();
            obj.put("num", i);
            obj.put("group", "hello");
            obj.put("int", rand.nextInt());
            obj.put("double", 1234.567);

            objs.add(obj);
        }

        cl.bulkInsert(objs, 0);
        assertEquals(n, cl.getCount());

        int sum = (n - 1) * n / 2;

        BSONObject group = new BasicBSONObject();
        group.put("_id", "$group");
        BSONObject sumObj = new BasicBSONObject();
        sumObj.put("$sum", "$num");
        group.put("sum", sumObj);

        BSONObject obj = new BasicBSONObject();
        obj.put("$group", group);

        List<BSONObject> list = new ArrayList<BSONObject>();
        list.add(obj);
        DBCursor cursor = cl.aggregate(list);
        assertTrue(cursor.hasNext());
        BSONObject result = cursor.getNext();
        int retSum = ((Double) result.get("sum")).intValue();
        assertEquals(sum, retSum);
        cursor.close();
    }
}
