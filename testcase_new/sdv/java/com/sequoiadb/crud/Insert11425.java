package com.sequoiadb.crud;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.Random;

/**
 * Created by laojingtang on 18-1-4.
 */
public class Insert11425 extends SdbTestBase {
    private Sequoiadb db = null;
    private static final String CLNAME = Insert11425.class.getSimpleName();
    private DBCollection dbcl;

    @BeforeClass
    public void setup() {
        db = new Sequoiadb(SdbTestBase.coordUrl, "", "");
        CollectionSpace cs = db.getCollectionSpace(SdbTestBase.csName);
        dbcl = cs.createCollection(CLNAME);
    }

    @AfterClass
    public void teardown() {
        if (db != null) {
            db.disconnect();
        }
    }

    /**
     * 1.多线程插入，同时删除cl
     */
    @Test
    public void test() throws InterruptedException {
        SdbThreadBase insert = new SdbThreadBase() {
            @Override
            public void exec() throws Exception {
                Sequoiadb db = null;
                try {
                    db = new Sequoiadb(SdbTestBase.coordUrl, "", "");
                    DBCollection cl = db.getCollectionSpace(SdbTestBase.csName)
                            .getCollection(CLNAME);
                    for (int i = 0; i < 10000; i++) {
                        cl.insert(new BasicBSONObject("b", i));
                    }
                } catch (BaseException e) {
                    if (e.getErrorCode() != -23) throw e;
                } finally {
                    if (db != null)
                        db.disconnect();
                }
            }
        };

        insert.start(20);

        Thread.sleep(300 + new Random().nextInt(200));
        CollectionSpace cs = db.getCollectionSpace(SdbTestBase.csName);
        cs.dropCollection(CLNAME);

        Assert.assertTrue(insert.isSuccess(), insert.getErrorMsg());
        Assert.assertFalse(cs.isCollectionExist(CLNAME));
    }
}
