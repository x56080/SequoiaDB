package com.sequoiadb.transaction;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbConfTestBase;
import com.sequoiadb.testcommon.SdbTestBase;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterTest;
import org.testng.annotations.BeforeTest;
import org.testng.annotations.Test;

import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;

/**
 * @author ljt
 * @version 1.00
 */
public class TestTransaction13632 extends SdbConfTestBase {
    private Sequoiadb sdb;
    private DBCollection cl;

    @Override
    protected void setNodeConf() {
        dataConf.put("transactionon", true);
        stdalnConf.put("transactionon", true);
    }

    @BeforeTest
    public void setUp() {
        System.out.println("the TestCase Name:" + this.getClass().getName() +
                ". the TestCase begin at:" + new SimpleDateFormat("YYYY-MM-dd HH:mm:ss.SSS").format(new Date()));
        sdb = new Sequoiadb(SdbTestBase.coordUrl, "", "");
        cl=sdb.getCollectionSpace(SdbTestBase.csName)
                .createCollection(TestTransaction13632.class.getSimpleName());
    }

    @Test
    public void testTransactionBeginCommit() {
        if (!Util.isCluster(sdb)) {
            throw new SkipException("It is not cluster");
        }
        List r;
        r = getSnap(Sequoiadb.SDB_SNAP_TRANSACTIONS);
        Assert.assertEquals(r.size(), 0);
        r = getSnap(Sequoiadb.SDB_SNAP_TRANSACTIONS_CURRENT);
        Assert.assertEquals(r.size(), 0);

        sdb.beginTransaction();
        cl.insert("{a:1}");
        r = getSnap(Sequoiadb.SDB_SNAP_TRANSACTIONS);
        Assert.assertTrue(r.size() > 0,"actual: "+r);
        r = getSnap(Sequoiadb.SDB_SNAP_TRANSACTIONS_CURRENT);
        Assert.assertTrue(r.size() > 0,"actual: "+r);
        sdb.commit();
    }

    private List getSnap(int code) {
        DBCursor cursor = sdb.getSnapshot(code, "", "", "");
        List r = new ArrayList(5);
        while (cursor.hasNext())
            r.add(cursor.getNext());
        return r;
    }

    @AfterTest
    public void tearDown() {
        System.out.println("the TestCase Name:" + this.getClass().getName() +
                ". the TestCase end at:" + new SimpleDateFormat("YYYY-MM-dd HH:mm:ss.SSS").format(new Date()));
        sdb.getCollectionSpace(SdbTestBase.csName)
                .dropCollection(cl.getName());
        sdb.disconnect();
    }
}
