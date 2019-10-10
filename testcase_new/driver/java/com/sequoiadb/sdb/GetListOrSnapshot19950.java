package com.sequoiadb.sdb;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Descreption seqDB-19950:getList新增hint/limit/skip参数，并新增list类型
 * @Author huangxiaoni
 * @Date 2019.10.10
 */

public class GetListOrSnapshot19950 extends SdbTestBase {
    private int runSuccNum = 0;
    private int expRunSuccNum = 2;
    private Sequoiadb sdb;
    private String clName = "cl19950";
    private String fullCLName;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb(SdbTestBase.coordUrl, "", "");
        sdb.getCollectionSpace(csName).createCollection(clName);
        fullCLName = csName + "." + clName;
    }

    @Test
    public void test_getList() {
        // test hint / skip / limit
        BSONObject query = new BasicBSONObject("Name", fullCLName);
        BSONObject hint = new BasicBSONObject("", "test");
        long skipRows = 0;
        long returnRows = 1;
        DBCursor cursor = sdb.getList(Sequoiadb.SDB_LIST_COLLECTIONS, query, null, null, hint, skipRows, returnRows);
        int size = 0;
        while (cursor.hasNext()) {
            Object name = cursor.getNext().get("Name");
            Assert.assertEquals(name, fullCLName);
            size++;
        }
        Assert.assertEquals(size, returnRows);

        // test listType: SDB_LIST_USERS, not need verify results
        cursor = sdb.getList(Sequoiadb.SDB_LIST_USERS, null, null, null);
        while (cursor.hasNext()) {
            cursor.getNext().get("User");
        }

        runSuccNum++;
    }

    @Test
    public void test_getSnapshot() {
        BSONObject query = new BasicBSONObject("Name", fullCLName);
        BSONObject hint = new BasicBSONObject("", "test");
        long skipRows = 0;
        long returnRows = 1;
        DBCursor cursor = sdb.getSnapshot(Sequoiadb.SDB_SNAP_COLLECTIONS, query, null, null, hint, skipRows,
                returnRows);
        int size = 0;
        while (cursor.hasNext()) {
            Object name = cursor.getNext().get("Name");
            Assert.assertEquals(name, fullCLName);
            size++;
        }
        Assert.assertEquals(size, returnRows);
        runSuccNum++;
    }

    @AfterClass
    public void tearDown() {
        try {
            if (runSuccNum < expRunSuccNum) {
                sdb.getCollectionSpace(csName).dropCollection(clName);
            }
        } finally {
            sdb.disconnect();
        }
    }
}
