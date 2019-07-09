package com.sequoiadb.split;

import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;
import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Node;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 *  创建cl时指定副本数为2，执行切分 
 * @author chensiqin
 * @Date 2016-12-16
 */
public class TestSplit10525C extends SdbTestBase{
    private Sequoiadb sdb;
    private CollectionSpace cs;
    private DBCollection cl;
    private String clName = "cl10525_2";
    private List<BSONObject> insertRecods;
    
    @BeforeClass
    public void setUp() {
        try{
            this.sdb = new Sequoiadb(SdbTestBase.coordUrl, "", "");
            // 跳过 standAlone 和数据组不足的环境
            SplitUtils2 util = new SplitUtils2();
            if (util.isStandAlone(this.sdb)) {
                throw new SkipException("skip StandAlone");
            }
            if (SplitUtils2.getDataRgNames(this.sdb).size() < 2) {
                throw new SkipException("current environment less than tow groups ");
            }
            BSONObject options = new BasicBSONObject();
            options.put("PreferedInstance", "M");
            this.sdb.setSessionAttr(options);
            this.cs = this.sdb.getCollectionSpace(SdbTestBase.csName);
        } catch (BaseException e) {
            Assert.fail(e.getMessage());
        }
    }

    /**
     * 1. 创建创建cl到指定组写部分节点，如2
     * 2. 分区键字段排序为逆序
     * 3. 执行异步切分
     * 4. 分别连接coord、源组data、目标组data查询
     */
    @Test
    public void test() {
        try {
            List<String> rgNames = SplitUtils2.getDataRgNames(this.sdb); 
            BSONObject option = (BSONObject) JSON.parse("{ReplSize:2,ShardingKey:{num:-1},ShardingType:\"range\",Group:\"" + rgNames.get(0) + "\"}");
            this.cl = SplitUtils2.createCL(this.cs, this.clName, option); 
            BSONObject startCondition = (BSONObject) JSON.parse("{num:0.63}");
            BSONObject endCondition = (BSONObject) JSON.parse("{num:0.31}");
            long splitAsyncId = this.cl.splitAsync(rgNames.get(0), rgNames.get(1), startCondition, endCondition);
            this.insertRecods = new ArrayList<BSONObject>();
            this.insertRecods = SplitUtils2.insertData(this.cl, 100);
            //等待切分任务完成再校验数据
            long[] taskIDs = {splitAsyncId};
            this.sdb.waitTasks(taskIDs);
            //连接coord节点验证数据是否正确
            testCoordSplitResult(rgNames);
            //连接源组data验证数据
            testSrcDataSplitResult(rgNames);
            //连接目标组data验证数据
            testDestDataSplitResult(rgNames);
        }catch (BaseException e) {
            e.printStackTrace();
            Assert.fail(e.getMessage());
        }
    }
    
    public void testCoordSplitResult(List<String> rgNames) {
    	List<BSONObject> actual = new ArrayList<BSONObject>();
        try {
            //连接coord节点验证数据是否正确
            DBCursor cursor = this.cl.query(null,null,"{\"_id\":1}",null);
            while( cursor.hasNext() ) {
                BSONObject obj = cursor.getNext();
                actual.add(obj);
            }
            cursor.close();
            Assert.assertEquals(actual, this.insertRecods);
        } catch (BaseException e) {
            Assert.fail(e.getMessage());
        }
    }

    public void testSrcDataSplitResult(List<String> rgNames) {
        Sequoiadb dataDb = null;
        try {
            //连接源组从节点data验证数据
            ReplicaGroup replicaGroup = this.sdb.getReplicaGroup(rgNames.get(0));
            Node master = replicaGroup.getSlave();
            String url = master.getNodeName();
            dataDb = new Sequoiadb(url, "", "");
            
            boolean flag = false;
            for (int j = 0; j < 100; j++) {
                //获取cs cl
                CollectionSpace cs = dataDb.getCollectionSpace(SdbTestBase.csName);
                DBCollection dbcl = null;
                try {
                    dbcl = cs.getCollection(this.clName);
                } catch (BaseException e1) {
                   if(e1.getErrorCode() == -23) {
                       continue;
                   } else {
                       Assert.fail(e1.getMessage());
                   }
                }
                try {
                    Thread.sleep(10);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
                //通过从节点查询数据
                DBCursor cursor;
                cursor = dbcl.query(null,null,"{\"_id\":1}",null);
                List<BSONObject> actual = new ArrayList<BSONObject>();
                while( cursor.hasNext() ) {
                    BSONObject obj = cursor.getNext();
                    actual.add(obj);
                }
                cursor.close();
                List<BSONObject> expected = new ArrayList<BSONObject>();
                //切分键[0.63-0.31)
                //期望结果[1-31],[64,100)
                for( int i = 1; i <= 31; i++ ) {
                    expected.add(this.insertRecods.get(i-1));
                }
                for( int i = 64; i < 100; i++ ) {
                    expected.add(this.insertRecods.get(i-1));
                }
                if ( actual.equals(expected) ) {
                    flag = true;
                }
            }
            if (!flag) {
                Assert.fail("数据长时间未同步成功！");
            }
           // Assert.assertEquals(actual, expected);
        } catch (BaseException e) {
            Assert.fail(e.getMessage());
        } finally {
            dataDb.disconnect();
        }
        
    }

    public void testDestDataSplitResult(List<String> rgNames) {
        Sequoiadb dataDb = null;
        try {
            //连接目标组data查询
            ReplicaGroup replicaGroup = this.sdb.getReplicaGroup(rgNames.get(1));
            Node master = replicaGroup.getSlave();
            String url = master.getNodeName();
            dataDb = new Sequoiadb(url, "", "");
            
            boolean flag = false;
            for (int j = 0; j < 100; j++){
                DBCollection dbcl = null;
                for ( int k = 0; k < 20; k++) {
                    //通过从节点获取cs cl
                    try {
                        CollectionSpace cs = dataDb.getCollectionSpace(SdbTestBase.csName);
                        dbcl = cs.getCollection(this.clName);
                    } catch (BaseException e1) {
                       if ( e1.getErrorCode() == -34 || e1.getErrorCode() == -23) {
                           continue;
                       }
                    }
                    try {
                        Thread.sleep(10);
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                }
                //通过从节点查询
                DBCursor cursor;
                try {
                    cursor = dbcl.query(null,null,"{\"_id\":1}",null);
                } catch (NullPointerException e) {
                    continue;
                }
                List<BSONObject> actual = new ArrayList<BSONObject>();
                while( cursor.hasNext() ) {
                    BSONObject obj = cursor.getNext();
                    actual.add(obj);
                }
                cursor.close();
                List<BSONObject> expected = new ArrayList<BSONObject>();
                //切分键[0.63-0.31)
                //期望结果[0.63-0.31)
                for( int i = 32; i <= 63; i++ ) {
                    expected.add(this.insertRecods.get(i-1));
                }
                if ( actual.equals(expected) ) {
                    flag = true;
                }
            }
            if (!flag) {
                Assert.fail("数据长时间未同步成功！");
            }
            //Assert.assertEquals(actual, expected);
        } catch (BaseException e) {
            throw e;
        } finally {
            dataDb.disconnect();
        }
    }
    
    
    @AfterClass
    public void tearDown() {
        try {
            if (this.cs.isCollectionExist(clName)) {
                this.cs.dropCollection(clName);
            }
        } catch (BaseException e) {
            Assert.fail(e.getMessage());
        } finally {
            this.sdb.disconnect();
        }
    }
}
