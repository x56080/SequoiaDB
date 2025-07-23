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

   Source File Name = BSONTimeStampTest.java

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

import org.bson.types.BSONTimestamp;
import org.junit.Assert;
import org.junit.Test;

import java.util.Date;
import java.util.HashMap;


public class BSONTimeStampTest {

    /**
    *  test timestamp hashcode in different situation
    * */
    @Test
    public void BSONTimeStampHashCodeTest() {
        BSONTimestamp actual = new BSONTimestamp();
        BSONTimestamp excepted = new BSONTimestamp();
        String actualStr = "actual";
        String exceptedStr = "excepted";
        HashMap<BSONTimestamp, String> map = new HashMap<>();

        // case1: two equals timestamp objects. they have same hashcode and hash container only have one record
        Assert.assertTrue(actual.equals(excepted));
        Assert.assertEquals(excepted.hashCode(), actual.hashCode());
        map.put(actual, actualStr);
        map.put(excepted, exceptedStr);
        Assert.assertEquals(1, map.size());
        Assert.assertEquals(exceptedStr, map.get(actual));
        Assert.assertEquals(exceptedStr, map.get(excepted));

        // case2: two variables use the same Object reference
        map.clear();
        BSONTimestamp reference = actual;
        Assert.assertTrue(actual.equals(reference));
        Assert.assertEquals(reference.hashCode(), actual.hashCode());
        map.put(actual, actualStr);
        map.put(reference, exceptedStr);
        Assert.assertEquals(1, map.size());
        Assert.assertEquals(exceptedStr, map.get(actual));
        Assert.assertEquals(exceptedStr, map.get(reference));

        // case3: diff timestamp objects. they have difference hashcode and hash container have two record
        map.clear();
        excepted = new BSONTimestamp(new Date());
        Assert.assertFalse(actual.equals(excepted));
        Assert.assertNotEquals(actual.hashCode(), excepted.hashCode());
        map.put(actual, actualStr);
        map.put(excepted, exceptedStr);
        Assert.assertEquals(2, map.size());
        Assert.assertEquals(actualStr, map.get(actual));
        Assert.assertEquals(exceptedStr, map.get(excepted));
    }
}
