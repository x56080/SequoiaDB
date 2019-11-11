package com.sequoiadb.crud;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

import java.util.Date;
import java.util.Random;
import java.util.concurrent.atomic.AtomicInteger;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BSONDecimal;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * FileName: InsertAndDropCS11426.java test content:concurrent insert, when drop
 * cs testlink case:seqDB-11426
 * 
 * @author wuyan
 * @Date 2017.11.9
 * @version 1.00
 */
public class InsertAndDropCS11426 extends SdbTestBase {
    final String CLNAME = "cl_11426";
    final String CSNAME = "cs_11426";
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection dbcl;
    private AtomicInteger thCount = new AtomicInteger();
    private int insertThNum = 20;

    @BeforeClass
    public void setup() {
        sdb = new Sequoiadb(SdbTestBase.coordUrl, "", "");
        if (sdb.isCollectionSpaceExist(CSNAME)) {
            sdb.dropCollectionSpace(CSNAME);
        }
        cs = sdb.createCollectionSpace(CSNAME);
        dbcl = cs.createCollection(CLNAME);
    }

    @Test
    public void test() throws InterruptedException {
        InsertTask insertTask = new InsertTask();
        insertTask.start(insertThNum);

        // sleep random value to drop cs at different stages of insert
        int time = new Random().nextInt(1000);
        Thread.sleep(time);

        // drop cs fail is -147, repeat drop cs again
        int eachSleepTime = 1000;
        int errorNo = 0;
        boolean breakFlag = false;
        do {
            if (thCount.get() == 20) {
                breakFlag = true;
            }
            Thread.sleep(eachSleepTime);
            errorNo = dropCS(sdb);
            if (breakFlag) {
                break;
            }
        } while (errorNo == -147);

        Assert.assertTrue(insertTask.isSuccess(), insertTask.getErrorMsg());

        // check the result
        Assert.assertFalse(sdb.isCollectionSpaceExist(CSNAME), "cs exist!!");
        // insert fail after drop cs
        try {
            dbcl.insert("{a:1}");
            Assert.fail("insert should be fail!");
        } catch (BaseException e) {
            if (e.getErrorCode() != -34 && e.getErrorCode() != -23) {
                Assert.fail("insert should be fail! e:" + e.getErrorCode());
            }
        }
    }

    @AfterClass
    public void teardown() {
        try {
            if (sdb.isCollectionSpaceExist(CSNAME)) {
                sdb.dropCollectionSpace(CSNAME);
            }
        } finally {
            if (sdb != null) {
                sdb.disconnect();
            }
        }

    }

    private int dropCS(Sequoiadb sdb) {
        int errorNo = 0;
        try {
            sdb.dropCollectionSpace(CSNAME);
        } catch (BaseException e) {
            errorNo = e.getErrorCode();
        }
        return errorNo;
    }

    private class InsertTask extends SdbThreadBase {
        @Override
        public void exec() throws Exception {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb(SdbTestBase.coordUrl, "", "");
                DBCollection cl = db.getCollectionSpace(CSNAME).getCollection(CLNAME);
                for (int i = 0; i < 10000; i++) {
                    BSONObject obj = new BasicBSONObject();
                    obj.put("a", i);
                    obj.put("str", "test_" + String.valueOf(i));
                    // insert the decimal type data
                    String str = "32345.067891234567890123456789" + i;
                    BSONDecimal decimal = new BSONDecimal(str);
                    obj.put("decimal", decimal);
                    // the data type
                    Date now = new Date();
                    obj.put("date", now);
                    cl.insert(obj);
                }
            } catch (BaseException e) {
                if (e.getErrorCode() != -34 && e.getErrorCode() != -23 && e.getErrorCode() != -248) {
                    System.out.println("---insert e=" + e.getErrorCode());
                    Assert.fail("insert fail!!");
                }
            } finally {
                if (db != null)
                    db.disconnect();
                thCount.incrementAndGet();
            }
        }
    }
}
