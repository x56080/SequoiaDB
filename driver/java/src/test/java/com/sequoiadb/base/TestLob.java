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
