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
