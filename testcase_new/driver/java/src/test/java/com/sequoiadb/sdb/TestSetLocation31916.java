package com.sequoiadb.sdb;

import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Node;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.ArrayList;

/**
 * @description seqDB-31916:验证node.setLocation() 接口
 * @author Cheng Jingjing
 * @date 2023.06.06
 * @version 1.10
 */

public class TestSetLocation31916 extends SdbTestBase {
    private static Sequoiadb sdb;
    private static Node node = null;
    private static ArrayList<String> groupNames;

    @BeforeClass
    public static void setUpBeforeClass() {
        sdb = new Sequoiadb(SdbTestBase.coordUrl, "", "");
        if (CommLib.isStandAlone(sdb)) {
            throw new SkipException("skip standalone.");
        }
        groupNames = CommLib.getDataGroupNames(sdb);
    }

    @Test
    public void test() {
        // get node
        node = sdb.getReplicaGroup(groupNames.get(0)).getMaster();
        try {
            node.setLocation(null);
            Assert.fail( "expect fail but success." );
        } catch (BaseException e) {
            Assert.assertEquals( SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode() );
        }

        setAndCheckLocation(node, "shanghai");

        setAndCheckLocation(node, "guangzhou.nansha");

        setAndCheckLocation(node, "china.guangzhou.nansha");

        setAndCheckLocation(node, "");
    }

    private void setAndCheckLocation(Node node, String location) {
        // 1. set location
        node.setLocation(location);

        // 2. query location
        String actualLocation = "";
        BSONObject matcher = new BasicBSONObject();
        matcher.put( "RawData", true );
        matcher.put("NodeID", node.getNodeId());


        BSONObject selector = new BasicBSONObject();
        selector.put("Location", "");

        try (DBCursor cursor = sdb.getSnapshot(Sequoiadb.SDB_SNAP_SYSTEM, matcher, null, null)){
            BSONObject obj = cursor.getNext();
            if (obj.containsField("Location")) {
                actualLocation = (String)obj.get("Location");
            }
        }

        // 3. check location
        Assert.assertEquals(actualLocation, location);
    }

    @AfterClass
    public static void tearDownAfterClass() {
        sdb.close();
    }
}
