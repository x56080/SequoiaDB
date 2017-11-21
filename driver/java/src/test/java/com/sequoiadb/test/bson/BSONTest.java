package com.sequoiadb.test.bson;


import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.junit.*;

import java.util.HashMap;
import java.util.Map;

import static org.junit.Assert.*;
import static org.junit.Assert.assertThat;

/**
 * Created by tanzhaobo on 2017/11/21.
 */
public class BSONTest {

    @BeforeClass
    public static void setConnBeforeClass() throws Exception {
    }

    @AfterClass
    public static void DropConnAfterClass() throws Exception {
    }

    @Before
    public void setUp() throws Exception {
    }

    @After
    public void tearDown() throws Exception {
    }

    @Test
    public void BSONHashCodeTest() {
        BSONObject obj1 = new BasicBSONObject().append("a", 1).append("b", new BasicBSONObject("b",1));
        BSONObject obj2 = new BasicBSONObject().append("a", 1).append("b", new BasicBSONObject("b",1));
        assertEquals(obj1, obj1);
        assertEquals(obj1, obj2);
        assertEquals(obj2, obj1);
        assertTrue (obj1.equals(obj1));
        assertTrue (obj1.equals(obj2));
        assertTrue (obj2.equals(obj1));
        assertEquals(obj1.hashCode(), obj1.hashCode());
        assertEquals(obj1.hashCode(), obj2.hashCode());
        assertEquals(obj2.hashCode(), obj1.hashCode());

    }

    @Test
    public void BSONInMapTest() {
        BSONObject obj1 = new BasicBSONObject().append("a", 1).append("b", new BasicBSONObject("b",1));
        BSONObject obj2 = new BasicBSONObject().append("a", 1).append("b", new BasicBSONObject("b",1));
        BSONObject obj3 = new BasicBSONObject().append("a", 1).append("b", new BasicBSONObject("b",3));
        BSONObject obj4 = new BasicBSONObject().append("a", 1).append("b", new BasicBSONObject("b",4));

        Map<BSONObject, BSONObject> map = new HashMap<>();
        map.put(obj1, obj1);
        map.put(obj2, obj2);
        map.put(obj3, obj3);
        map.put(obj4, obj4);
        assertEquals(3, map.size());
    }

}
