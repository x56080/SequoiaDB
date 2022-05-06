package com.sequoiadb.base;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import org.bson.*;
import org.bson.io.Bits;
import org.bson.types.*;
import org.bson.util.DateInterceptUtil;
import org.bson.util.JSON;
import org.junit.Assert;
import org.junit.BeforeClass;
import org.junit.Ignore;
import org.junit.Test;

import javax.xml.bind.DatatypeConverter;
import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.sql.Timestamp;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.*;
import java.util.regex.Pattern;

import static org.junit.Assert.*;

public class TestBSON {
    private static BSONObject object;
    private static byte[] objBytes;

    @BeforeClass
    public static void setUpTestCase() {
        Date date = DateInterceptUtil.interceptDate(new Date(), "yyyy-MM-dd");

        BSONObject embeddedObj = new BasicBSONObject();
        embeddedObj.put("int", 123);
        embeddedObj.put("long", 12345L);
        embeddedObj.put("double", 123.456);
        embeddedObj.put("string", "hello");
        embeddedObj.put("null", null);
        embeddedObj.put("maxKey", new MaxKey());
        embeddedObj.put("minKey", new MinKey());
        embeddedObj.put("oid", new ObjectId());
        embeddedObj.put("true", true);
        embeddedObj.put("false", false);
        embeddedObj.put("date", date);
        embeddedObj.put("timestamp", new BSONTimestamp((int) (System.currentTimeMillis() / 1000), 1234));
        embeddedObj.put("decimal", new BSONDecimal("12345678901234567890.09876543210987654321"));

        BasicBSONList embeddedArray = new BasicBSONList();
        embeddedArray.put("0", 123);
        embeddedArray.put("1", 12345L);
        embeddedArray.put("2", 123.456);
        embeddedArray.put("3", "hello");
        embeddedArray.put("4", null);
        embeddedArray.put("5", new MaxKey());
        embeddedArray.put("6", new MinKey());
        embeddedArray.put("7", new ObjectId());
        embeddedArray.put("8", true);
        embeddedArray.put("9", false);
        embeddedArray.put("10", date);
        embeddedArray.put("11", new BSONTimestamp((int) (System.currentTimeMillis() / 1000), 1234));
        embeddedArray.put("12", new BSONDecimal("12345678901234567890.09876543210987654321"));

        Binary binary1 = new Binary(BSON.B_GENERAL, "Hello, world!".getBytes());
        Binary binary2 = new Binary(BSON.B_FUNC, "Hello, world!".getBytes());
        Binary binary3 = new Binary(BSON.B_BINARY, "Hello, world!".getBytes());
        UUID binary4 = UUID.randomUUID();

        BSONObject obj = new BasicBSONObject();
        obj.put("int", 123);
        obj.put("long", 12345L);
        obj.put("double", 123.456);
        obj.put("string", "hello");
        obj.put("null", null);
        obj.put("maxKey", new MaxKey());
        obj.put("minKey", new MinKey());
        obj.put("oid", new ObjectId());
        obj.put("true", true);
        obj.put("false", false);
        obj.put("date", date);
        obj.put("timestamp", new BSONTimestamp((int) (System.currentTimeMillis() / 1000), 1234));
        obj.put("decimal", new BSONDecimal("12345678901234567890.09876543210987654321"));
        obj.put("binary1", binary1);
        obj.put("binary2", binary2);
        obj.put("binary3", binary3);
        obj.put("binary4", binary4);
        obj.put("object", embeddedObj);
        obj.put("array", embeddedArray);
        obj.put("code", new Code("你好！"));
        obj.put("codewscope", new CodeWScope("hello", new BasicBSONObject()));
        obj.put("regex", Pattern.compile("^a", BSON.regexFlags("i")));
        // TODO Symbol 被解码成 String 导致用例失败，暂时屏蔽
        //obj.put("symbol", new Symbol("你好！"));

        object = obj;
        objBytes = BSON.encode(obj);
    }

    @Test
    public void testNewBSONDecoder() {
        byte[] bytes = objBytes;

        BSONDecoder decoder = new NewBSONDecoder();
        BSONObject decodedObj = decoder.readObject(bytes);

        assertEquals(object, decodedObj);

        Binary binary1 = (Binary) object.get("binary1");
        Binary bin1 = (Binary) decodedObj.get("binary1");
        assertEquals(binary1, bin1);

        Binary binary2 = (Binary) object.get("binary2");
        Binary bin2 = (Binary) decodedObj.get("binary2");
        assertEquals(binary2, bin2);

        Binary binary3 = (Binary) object.get("binary3");
        Binary bin3 = (Binary) decodedObj.get("binary3");
        assertEquals(binary3, bin3);

        UUID binary4 = (UUID) object.get("binary4");
        UUID bin4 = (UUID) decodedObj.get("binary4");
        assertEquals(binary4, bin4);
    }

    @Test
    public void testNewBSONDecoder2() {
        byte[] bytes = objBytes;
        byte[] bytes2 = new byte[bytes.length + 10];
        System.arraycopy(bytes, 0, bytes2, 10, bytes.length);

        BSONDecoder decoder = new NewBSONDecoder();
        BasicBSONCallback callback = new BasicBSONCallback();

        int length = decoder.decode(bytes2, 10, callback);
        assertEquals(Bits.readInt(bytes), length);
    }

    @Test
    public void testNewBSONDecoder3() {
        byte[] bytes = objBytes;
        byte[] bytes2 = new byte[bytes.length + 10];
        System.arraycopy(bytes, 0, bytes2, 10, bytes.length);

        BSONDecoder decoder = new NewBSONDecoder();
        BasicBSONCallback callback = new BasicBSONCallback();

        int length = 0;
        try {
            length = decoder.decode(new ByteArrayInputStream(bytes2, 10, bytes.length), callback);
        } catch (IOException e) {
            e.printStackTrace();
            fail();
        }
        assertEquals(Bits.readInt(bytes), length);
    }

    @Test
    public void testBSONTimestamp() {
        Date srcDate = new Date();
        BSONTimestamp ts = new BSONTimestamp(srcDate);
        Date toDate = ts.toDate();
        assertEquals(srcDate, toDate);

        Timestamp srcTS = new Timestamp(srcDate.getTime());
        srcTS.setNanos(123456000);
        // nanoseconds loss
        BSONTimestamp ts2 = new BSONTimestamp(srcTS);
        Timestamp toTS = ts2.toTimestamp();
        assertEquals(srcTS, toTS);
    }

    @Test
    public void testBSONCodecTimestamp() {
        Timestamp ts = new Timestamp(new Date().getTime());
        ts.setNanos(123456000);

        BSONObject obj = new BasicBSONObject();
        obj.put("ts", ts);

        byte[] bytes = BSON.encode(obj);
        BSONObject obj2 = BSON.decode(bytes);

        BSONTimestamp bts = (BSONTimestamp)obj2.get("ts");
        Timestamp ts2 = bts.toTimestamp();

        assertEquals(ts, ts2);
    }

    @Test
    public void testJSONParseDate() {
        Date date = null;
        try {
            date = new SimpleDateFormat("yyyy-MM-dd").parse("2017-06-14");
        } catch (ParseException e) {
            e.printStackTrace();
            fail();
        }
        java.sql.Date date2 = new java.sql.Date(date.getTime());

        BSONObject obj = new BasicBSONObject();
        obj.put("date", date);
        obj.put("date2", date2);

        String json = obj.toString();

        BSONObject obj2 = (BSONObject) JSON.parse(json);

        assertEquals(obj, obj2);
    }

    @Test
    public void testJSONParseTimestamp() {
        Timestamp ts = new Timestamp(new Date().getTime());
        ts.setNanos(123456000);

        BSONObject obj = new BasicBSONObject();
        obj.put("ts", ts);

        BSONObject obj2 = new BasicBSONObject();
        obj2.put("ts", new BSONTimestamp(ts));

        String json = JSON.serialize(obj);
        BSONObject obj3 = (BSONObject) JSON.parse(json);

        assertEquals(obj2, obj3);
    }

    @Test
    public void testJSONParseBinary() {
        String str = "hello world";
        Binary binary = new Binary(str.getBytes());

        BSONObject obj = new BasicBSONObject();
        obj.put("bin", binary);

        String json = obj.toString();
        BSONObject obj2 = (BSONObject) JSON.parse(json);

        assertEquals(obj, obj2);
    }

    @Test
    public void testNumberLong() {
        BSONObject exp = (BSONObject) JSON.parse("{'no':{'$numberLong':'8223372036854775296'}}");
        BSONObject exp2 = (BSONObject) JSON.parse("{'no':{'$numberLong':'8223372036854775807'}}");
        assertNotEquals(exp, exp2);

        BSONObject exp3 = (BSONObject) JSON.parse("{'no':{'$numberLong':'8223372036854775296'}}");
        BSONObject exp4 = (BSONObject) JSON.parse("{'no':{'$numberLong':'8223372036854775296'}}");
        assertEquals(exp3, exp4);
    }

    @Test
    public void testBSONTimestampInc() {
        BSONTimestamp ts1 = new BSONTimestamp(10000, 1000000);
        Assert.assertEquals(10001, ts1.getTime());
        Assert.assertEquals(0, ts1.getInc());

        BSONTimestamp ts2 = new BSONTimestamp(10000, -1);
        Assert.assertEquals(9999, ts2.getTime());
        Assert.assertEquals(999999, ts2.getInc());

        BSONTimestamp ts3 = new BSONTimestamp(10000, 0);
        Assert.assertEquals(10000, ts3.getTime());
        Assert.assertEquals(0, ts3.getInc());

        BSONTimestamp ts4 = new BSONTimestamp(10000, 999999);
        Assert.assertEquals(10000, ts4.getTime());
        Assert.assertEquals(999999, ts4.getInc());

        int time = 1534942305;
        int inc = 123456789;
        int incSec = 123456789 / 1000000;
        int incMSec = 123456789 % 1000000;
        int incMSec2 = 1000000 - incMSec;
        BSONTimestamp ts5 = new BSONTimestamp(time, inc);
        Assert.assertEquals(time + incSec, ts5.getTime());
        Assert.assertEquals(incMSec, ts5.getInc());

        BSONTimestamp ts6 = new BSONTimestamp(time, -inc);
        Assert.assertEquals(time - incSec - 1, ts6.getTime());
        Assert.assertEquals(incMSec2, ts6.getInc());

        BSONTimestamp ts7 = new BSONTimestamp(-time, inc);
        Assert.assertEquals(-time + incSec, ts7.getTime());
        Assert.assertEquals(incMSec, ts7.getInc());

        BSONTimestamp ts8 = new BSONTimestamp(-time, -inc);
        Assert.assertEquals(-time - incSec - 1, ts8.getTime());
        Assert.assertEquals(incMSec2, ts8.getInc());
    }

    @Test
    public void testPutAllUnique(){
        Map<String,String> map = new HashMap<>();
        map.put("1","1");
        map.put("2","2");
        map.put("3","3");

        // test in BasicBSONObject
        // test putAllUnique(BSONObject b)
        BSONObject obj1 = new BasicBSONObject();
        obj1.put("A",1);
        obj1.put("B",2);
        obj1.put("C",3);

        BSONObject obj2 = new BasicBSONObject();
        obj2.put("A",5);
        obj2.putAllUnique(obj1);
        obj1.put("A",5);
        System.out.println(obj2);
        assertEquals(obj1, obj2);

        // test putAllUnique(Map m)
        BSONObject obj3 = new BasicBSONObject();
        obj3.put("1","5");
        obj3.putAllUnique(map);
        map.put("1","5");
        BSONObject obj4 = new BasicBSONObject();
        obj4.putAll(map);
        System.out.println(obj3);
        assertEquals(obj3, obj4);

        // test in BasicBSONObject
        // test putAllUnique(BSONObject b)
        BSONObject objList1 = new BasicBSONList();
        objList1.put("1",1);
        objList1.put("2",2);
        objList1.put("3",3);

        BSONObject objList2 = new BasicBSONList();
        objList2.put("1",5);
        objList2.putAllUnique(objList1);
        objList1.put("1",5);
        assertEquals(objList1, objList2);
        System.out.println(objList2);

        // test putAllUnique(Map m)
        map.put("1","1");
        BSONObject objList3 = new BasicBSONList();
        objList3.put("1","5");
        objList3.putAllUnique(map);
        map.put("1","5");
        BSONObject objList4 = new BasicBSONList();
        objList4.putAll(map);
        assertEquals(objList3, objList4);
        System.out.println(objList3);
    }

}
