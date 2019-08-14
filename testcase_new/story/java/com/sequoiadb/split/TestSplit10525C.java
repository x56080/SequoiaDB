package com.sequoiadb.split;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 *  创建cl时指定副本数为2，执行切分 
 * @author chensiqin
 * @Date 2016-12-16
 */
public class TestSplit10525C extends SdbTestBase{
    private Sequoiadb sdb;
    private ArrayList<String> rgNames;
    private CollectionSpace cs;
    private DBCollection cl;
    private String clName = "cl10525_C";
    private List<BSONObject> insertRecods = new ArrayList<BSONObject>();
    
    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb(SdbTestBase.coordUrl, "", "");

        if (CommLib.isStandAlone(sdb) || CommLib.OneGroupMode(sdb)) {
            throw new SkipException("The mode is standlone, or only one group, "
                    + "skip the testCase.");
        }
        
        rgNames = CommLib.getDataGroupNames(sdb);
        
        cs = sdb.getCollectionSpace(SdbTestBase.csName);
        BSONObject option = (BSONObject) JSON.parse("{ReplSize:2,ShardingKey:{num:-1},"
                + "ShardingType:\"range\",Group:\"" + rgNames.get(0) + "\"}");
        cl = SplitUtils2.createCL(cs, clName, option);
    }

    /**
     * 1. 创建cl到指定组写部分节点，如2
     * 2. 分区键字段排序为逆序
     * 3. 执行异步切分
     * 4. 分别连接coord、源组data、目标组data查询
     * @throws InterruptedException 
     */
    @Test
    public void test() throws InterruptedException {
        BSONObject startCondition = (BSONObject) JSON.parse("{num:0.63}");
        BSONObject endCondition = (BSONObject) JSON.parse("{num:0.31}");
        long splitAsyncId = cl.splitAsync(rgNames.get(0), rgNames.get(1), 
                startCondition, endCondition);
        
        insertRecods = SplitUtils2.insertData(cl, 100);
        
        //等待切分任务完成再校验数据
        long[] taskIDs = {splitAsyncId};
        this.sdb.waitTasks(taskIDs);
        
        //连接coord节点验证数据是否正确
        testCoordSplitResult();
        
        //连接源组data验证数据
        List<BSONObject> srcExpectData = this.getSrcExpectData();
        checkSplitResult(rgNames.get(0), srcExpectData);
        
        //连接目标组data验证数据
        List<BSONObject> dstExpectData = this.getDstExpectData();
        checkSplitResult(rgNames.get(1), dstExpectData);
    }
    
    public void testCoordSplitResult() {
    	List<BSONObject> actual = new ArrayList<BSONObject>();
        DBCursor cursor = cl.query(null,null,"{\"_id\":1}",null);
        while( cursor.hasNext() ) {
            BSONObject obj = cursor.getNext();
            actual.add(obj);
        }
        cursor.close();
        Assert.assertEquals(actual, insertRecods);
    }

    public void checkSplitResult(String rgName, List<BSONObject> expData) 
            throws InterruptedException {
        ReplicaGroup replicaGroup = this.sdb.getReplicaGroup(rgName);
        Sequoiadb dataDb = null;
        DBCollection dbcl = null;
        try {
            // 连接源组从节点data验证数据
            dataDb = replicaGroup.getSlave().connect();
            boolean clFlag = false;
            for ( int i = 0; i < 100; i++) {
                // 元数据
                try {
                    CollectionSpace cs = dataDb.getCollectionSpace(SdbTestBase.csName);
                    dbcl = cs.getCollection(clName);
                    clFlag = true;
                    break;
                } catch (BaseException e) {
                   if ( e.getErrorCode() == -34 || e.getErrorCode() == -23) {
                       Thread.sleep(300);
                       continue;
                   } 
                   throw e;
                }
            }
            if (!clFlag) {
                Assert.fail("元数据长时间未同步成功！未同步节点：" + dataDb.getServerAddress());
            }
            
            // 数据
            boolean flag = false; 
            for (int j = 0; j < 100; j++) {  
                DBCursor cursor = dbcl.query(null,null,"{\"_id\":1}",null);
                List<BSONObject> actual = new ArrayList<BSONObject>();
                while( cursor.hasNext() ) {
                    BSONObject obj = cursor.getNext();
                    actual.add( obj );
                }                
                if ( actual.equals( expData ) ) {
                    flag = true;
                    break;
                } else {
                    Thread.sleep(300);
                }
            }
            if (!flag) {
                Assert.fail("数据长时间未同步成功！" + dataDb.getServerAddress());
            }
        } finally {
            dataDb.disconnect();
        }
    }
    
    private List<BSONObject> getSrcExpectData() {
        //切分键[0.63-0.31)
        //期望结果[1-31],[64,100)
        List<BSONObject> expData = new ArrayList<BSONObject>();
        for( int i = 1; i <= 31; i++ ) {
            expData.add(insertRecods.get(i-1));
        }
        for( int i = 64; i < 100; i++ ) {
            expData.add(insertRecods.get(i-1));
        }
        return expData;
    }
    
    private List<BSONObject> getDstExpectData() {
        //切分键[0.63-0.31)
        //期望结果[0.63-0.31)
        List<BSONObject> expData = new ArrayList<BSONObject>();
        for( int i = 32; i <= 63; i++ ) {
            expData.add(insertRecods.get(i-1));
        }
        return expData;
    }
    
    @AfterClass
    public void tearDown() {
        try {
            if (cs.isCollectionExist(clName)) {
                cs.dropCollection(clName);
            }
        } finally {
            this.sdb.disconnect();
        }
    }
}
