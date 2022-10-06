package com.sequoiadb.test.lob;

import com.sequoiadb.base.*;
import com.sequoiadb.datasource.SequoiadbDatasource;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.test.common.Constants;
import com.sequoiadb.test.common.Helper;
import com.sequoiadb.testdata.SDBTestHelper;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;
import org.junit.*;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.util.Arrays;
import java.util.Random;

import static org.junit.Assert.assertEquals;

public class DBLobSnapshotTest {
    private static Sequoiadb sdb;
    private static CollectionSpace cs;
    private static DBCollection cl;
    private static SequoiadbDatasource ds;

    private static final String LOB_SIZE = "Size";
    private static final String LOB_AVAILABLE = "Available";
    private static final String LOB_OID = "Oid";

    private String writeData = "abcdefghijklmnopqrstuvwxyz0123456789";
    private String data4B = "0123";
    private String data10B = "0123456789";
    private String data20B = data10B + data10B;
    private String data40B = data20B + data20B;
    private String data100B = data40B + data40B + data20B;
    private String data200B = data100B + data100B;
    private String data500B = data200B + data200B + data100B;

    private String data1000B = data500B + data500B;
    private String data1kb = data1000B;
    private String data2kb = data1000B + data1000B;
    private String data4kb = data2kb + data2kb;
    private String data8kb = data4kb + data4kb;
    private String data10kb = data4kb + data4kb + data2kb;
    private String data20kb = data10kb + data10kb;
    private String data50kb = data20kb + data20kb + data10kb;
    private String data100kb = data50kb + data50kb;
    private String data200kb = data100kb + data100kb;
    private String data500kb = data200kb + data200kb + data100kb;
    private String data1mb = data500kb + data500kb;
    private String data2mb = data1mb + data1mb;
//    private String data16kb = data8kb + data8kb;
//    private String data32kb = data16kb + data16kb;
//    private String data64kb = data32kb + data32kb;
//    private String data128kb = data64kb + data64kb;
//    private String data256kb = data128kb + data128kb;
//    private String data512kb = data256kb + data256kb;
//    private String data1mb = data512kb + data512kb;
//    private String data2mb = data1mb + data1mb;

    private String data1KB = data1000B + data20B + data4B;
    private String data2KB = data1KB + data1KB;
    private String data4KB = data2KB + data2KB;
    private String data8KB = data4KB + data4KB;
    private String data16KB = data8KB + data8KB;
    private String data32KB = data16KB + data16KB;
    private String data64KB = data32KB + data32KB;
    private String data128KB = data64KB + data64KB;
    private String data256KB = data128KB + data128KB;
    private String data512KB = data256KB + data256KB;
    private String data1MB = data512KB + data512KB;
    private String data2MB = data1MB + data1MB;


    @BeforeClass
    public static void setConnBeforeClass() throws Exception {

    }

    @AfterClass
    public static void DropConnAfterClass() throws Exception {

    }

    @Before
    public void setUp() throws Exception {
        ds = new SequoiadbDatasource(Arrays.asList(Constants.COOR_NODE_CONN),
                "admin", "admin", null, null);
        // sdb
        sdb = new Sequoiadb(Constants.COOR_NODE_CONN, "admin", "admin");
        // cs
        if (sdb.isCollectionSpaceExist(Constants.TEST_CS_NAME_1)) {
            sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
            cs = sdb.createCollectionSpace(Constants.TEST_CS_NAME_1);
        } else {
            cs = sdb.createCollectionSpace(Constants.TEST_CS_NAME_1);
        }
        // cl
        BSONObject conf = new BasicBSONObject();
        conf.put("ReplSize", 0);
        conf.put("Group", Constants.GROUPNAME);
        cl = cs.createCollection(Constants.TEST_CL_NAME_1, conf);
    }

    @After
    public void tearDown() throws Exception {
        try {
            //sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
            sdb.close();
        } catch (BaseException e) {
            e.printStackTrace();
        }
        if (ds != null) {
            ds.close();
        }
    }

    @Test
    public void testLobMode() {
        byte readData[] ;
        ObjectId oid;
        int seq = 1;

        // case 1: create empty lob
        {
            sdb.msg("case begin: " + seq);
            DBLob baseLob = cl.createLob();
            baseLob.close();
            oid = baseLob.getID();
            sdb.msg("case end: " + seq++);
        }

        // case 2, write to an exist lob.
        {
            sdb.msg("case begin: " + seq);
            DBLob lob1 = cl.openLob(oid, DBLob.SDB_LOB_WRITE);
            try {
                lob1.write(writeData.getBytes());
            } catch (BaseException e) {
                Assert.assertEquals(0, e.getErrorCode());
            } finally {
                lob1.close();
            }
            sdb.msg("case end: " + seq++);
        }

        // case 3, write to an newly created lob.
        {
            sdb.msg("case begin: " + seq);
            DBLob lob2 = cl.createLob();
            try {
                lob2.write(writeData.getBytes());
            } catch (BaseException e) {
                Assert.assertEquals(0, e.getErrorCode());
            } finally {
                lob2.close();
            }
            sdb.msg("case end: " + seq++);
        }

        // case 4, read from an exist lob.
        {
            sdb.msg("case begin: " + seq);
            DBLob lob3 = cl.openLob(oid, DBLob.SDB_LOB_READ);
            try {
                readData = new byte[(int) lob3.getSize()];
                lob3.read(readData);
            } catch (BaseException e) {
                Assert.assertEquals(0, e.getErrorCode());
            } finally {
                lob3.close();
            }
            sdb.msg("case end: " + seq++);
        }

        // case 5, read from an exist lob.
        {
            sdb.msg("case begin: " + seq);
            DBLob lob4 = cl.openLob(oid, DBLob.SDB_LOB_SHAREREAD);
            try {
                readData = new byte[(int) lob4.getSize()];
                lob4.read(readData);
            } catch (BaseException e) {
                Assert.assertEquals(0, e.getErrorCode());
            } finally {
                lob4.close();
            }
            sdb.msg("case end: " + seq++);
        }

        // case 6, open lob with write mode, read data from lob after write.
        {
            sdb.msg("case begin: " + seq);
            DBLob lob5 = cl.openLob(oid, DBLob.SDB_LOB_SHAREREAD | DBLob.SDB_LOB_WRITE);
            try {
                lob5.write(writeData.getBytes());
                readData = new byte[(int) lob5.getSize()];
                lob5.read(readData);
            } catch (BaseException e) {
                Assert.assertEquals(0, e.getErrorCode());
            } finally {
                lob5.close();
            }
            sdb.msg("case end: " + seq++);
        }

        // case 7, truncate lob.
        {
            sdb.msg("case begin: " + seq);
            cl.truncateLob(oid, 10);
            sdb.msg("case end: " + seq++);
        }

        // case 8, remove lob.
        {
            sdb.msg("case begin: " + seq);
            cl.removeLob(oid);
            sdb.msg("case end: " + seq++);
        }
    }

    @Test
    public void testLobAlternate() {
        DBLob lob1 = cl.createLob();
        DBLob lob2 = cl.createLob();
        byte[] bytes = data2mb.getBytes();
        System.out.println("bytes is: " + bytes.length);
        lob1.write(bytes);
        lob1.close();
        lob2.write(bytes);
        lob2.close();
    }

    @Test
    public void testOpenReadWrite() {

        byte[] outBytes = new byte[2000];
        byte[] inBytes = data2mb.getBytes();
        System.out.println("bytes is: " + inBytes.length);
        DBLob lob = cl.createLob();
        lob.write(inBytes);
        lob.close();

        DBLob lob1;
        DBLob lob2;
        lob1 = cl.openLob(lob.getID(),
                DBLob.SDB_LOB_SHAREREAD|DBLob.SDB_LOB_WRITE);
        lob2 = cl.openLob(lob.getID(),
                DBLob.SDB_LOB_SHAREREAD|DBLob.SDB_LOB_WRITE);
//        lob1.lockAndSeek(0, outBytes.length);
//        lob1.read(outBytes);  // totalReadSize = 1000
//        lob1.seek(1000, DBLob.SDB_LOB_SEEK_CUR);
//        lob1.lockAndSeek(outBytes.length, outBytes.length);
//        lob1.read(outBytes);  // totalReadSize = 2000
        lob2.lockAndSeek(inBytes.length, inBytes.length);
        lob2.write(inBytes);    // totalReadSize = 1000
        lob1.close();
        lob2.close();

        lob1 = cl.openLob(lob.getID(),
                DBLob.SDB_LOB_SHAREREAD);
        lob2 = cl.openLob(lob.getID(),
                    DBLob.SDB_LOB_WRITE);
        System.out.println("lob1 size: " + lob1.getSize());
        System.out.println("lob2 size: " + lob2.getSize());

    }


}
