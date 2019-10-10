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

public class GetList19950 extends SdbTestBase {
    private boolean runSuccess = false;
    private Sequoiadb sdb;
    private String clName = "cl19950";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb(SdbTestBase.coordUrl, "", "");
        sdb.getCollectionSpace(csName).createCollection(clName);
    }

    @Test
    public void test() {
        String fullCLName = csName + "." + clName;
        int listType = Sequoiadb.SDB_LIST_COLLECTIONS;
        BSONObject query = new BasicBSONObject("Name", fullCLName);
        BSONObject hint = new BasicBSONObject("", "test");
        long skipRows = 0;
        long returnRows = 1;
        DBCursor cursor = sdb.getList(listType, query, null, null, hint, skipRows, returnRows);
        int size = 0;
        while (cursor.hasNext()) {
            Object name = cursor.getNext().get("Name");
            Assert.assertEquals(name, fullCLName);
            size++;
        }
        Assert.assertEquals(size, returnRows);
        runSuccess = true;
    }

    @AfterClass
    public void tearDown() {
        try {
            if (runSuccess) {
                sdb.getCollectionSpace(csName).dropCollection(clName);
            }
        } finally {
            sdb.disconnect();
        }
    }
}
