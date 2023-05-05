package org.sequoiadb.tool;

import org.junit.*;
import com.sequoiadb.base.*;

import static org.junit.Assert.assertTrue;

public class CreateTable {

    private static Sequoiadb sdb;
    private static CollectionSpace cs;
    private static DBCollection cl;
    private static ReplicaGroup rg;
    private static Node node;
    private static DBCursor cursor;

    @BeforeClass
    public static void setConnBeforeClass() throws Exception {
        // sdb
        sdb = new Sequoiadb(Constants.COOR_NODE_CONN, "", "");
    }

    @AfterClass
    public static void DropConnAfterClass() throws Exception {
        sdb.close();
    }

    @Before
    public void setUp() throws Exception {
//        // cs
//        if (sdb.isCollectionSpaceExist(Constants.TEST_CS_NAME_1)) {
//            sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
//            cs = sdb.createCollectionSpace(Constants.TEST_CS_NAME_1);
//        } else
//            cs = sdb.createCollectionSpace(Constants.TEST_CS_NAME_1);
//        // cl
//        cl = cs.createCollection(Constants.TEST_CL_NAME_1, conf);
    }

    @After
    public void tearDown() throws Exception {
//        sdb.dropCollectionSpace(Constants.TEST_CS_NAME_1);
    }

    @Test
    public void createMainSubCL() {
        /*
        db.createCS("main2020").createCL("main", {IsMainCL:true, ShardingKey:{a:1}, ShardingType:"range"})
        db.createCS("main2021").createCL("main", {IsMainCL:true, ShardingKey:{a:1}, ShardingType:"range"})
        db.createCS("sub", {Domain:"domain2"}).createCL("sub202011", {ShardingKey:{a:1}, ShardingType:"hash", AutoSplit: true})
        db.sub.createCL("sub202012", {ShardingKey:{a:1}, ShardingType:"hash", AutoSplit: true})
        db.sub.createCL("sub202101", {ShardingKey:{a:1}, ShardingType:"hash", AutoSplit: true})
        db.sub.createCL("sub202102", {ShardingKey:{a:1}, ShardingType:"hash", AutoSplit: true})

        db.main2020.main.attachCL("sub.sub202011", {LowBound:{a:0}, UpBound:{a:100}})
        db.main2020.main.attachCL("sub.sub202012", {LowBound:{a:100}, UpBound:{a:200}})
        db.main2021.main.attachCL("sub.sub202101", {LowBound:{a:200}, UpBound:{a:300}})
        db.main2021.main.attachCL("sub.sub202102", {LowBound:{a:300}, UpBound:{a:400}})
        */
    }

}
