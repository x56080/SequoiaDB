package com.sequoiadb.split;

import java.util.ArrayList;
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
 *  创建cl时指定副本数为1，执行切分 
 * @author chensiqin
 * @Date 2016-12-16
 */
public class TestSplit10525B extends SdbTestBase{
    private Sequoiadb sdb;
    private CollectionSpace cs;
    private DBCollection cl;
    private String clName = "cl10525_1";
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
     * 1. 创建创建cl到指定组写1个节点，如1
     * 2. 分区键字段排序为逆序
     * 3. 执行异步切分
     * 4. 分别连接coord、源组data、目标组data查询
     * @throws InterruptedException 
     */
    @Test
    public void test() throws InterruptedException {
        try {
            List<String> rgNames = SplitUtils2.getDataRgNames(this.sdb); 
            BSONObject option = (BSONObject) JSON.parse("{ReplSize:1,ShardingKey:{age:-1},ShardingType:\"range\",Group:\"" + rgNames.get(0) + "\"}");
            this.cl = SplitUtils2.createCL(this.cs, this.clName, option); 
            BSONObject startCondition = (BSONObject) JSON.parse("{age:72}");
            BSONObject endCondition = (BSONObject) JSON.parse("{age:31}");
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
        try {
            //连接coord节点验证数据是否正确
            List<BSONObject> actual = new ArrayList<BSONObject>();
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

    public void testSrcDataSplitResult(List<String> rgNames) throws InterruptedException {
        ReplicaGroup replicaGroup = this.sdb.getReplicaGroup(rgNames.get(0));
        Sequoiadb dataDb = null;
        try {
            //连接源组备节点data验证数据
            dataDb = replicaGroup.getSlave().connect();
            
            DBCollection dbcl = null; 
            boolean clFlag = false;
            for ( int i = 0; i < 20; i++) {
                //通过从节点获取cs cl
                try {
                    CollectionSpace cs = dataDb.getCollectionSpace(SdbTestBase.csName);
                    dbcl = cs.getCollection(this.clName);
                    clFlag = true;
                    break;
                } catch (BaseException e) {
                   if ( e.getErrorCode() == -34 || e.getErrorCode() == -23) {
                       Thread.sleep(100);
                       continue;
                   } 
                   throw e;
                }
            }
            if (!clFlag) {
                Assert.fail("元数据长时间未同步成功！未同步节点：" + dataDb.getServerAddress());
            }
            
            //通过备节点查询数据
            //切分键[72-31)
            //期望结果[1-31],[73,100)             
            boolean flag = false;  
            List<BSONObject> expected = new ArrayList<BSONObject>();             
            for( int i = 1; i <= 31; i++ ) {
                expected.add(this.insertRecods.get(i-1));
            }
            for( int i = 73; i < 100; i++ ) {
                expected.add(this.insertRecods.get(i-1));
            }
            for (int j = 0; j < 100; j++) {  
                DBCursor cursor = dbcl.query(null,null,"{\"_id\":1}",null);                
                List<BSONObject> actual = new ArrayList<BSONObject>();
                while( cursor.hasNext() ) {
                    BSONObject obj = cursor.getNext();
                    actual.add( obj );
                }                
                if ( actual.equals( expected ) ) {
                    flag = true;
                    break;
                } else {
                    Thread.sleep(100);
                    continue;
                }
            }  
            if (!flag) {
                Assert.fail("数据长时间未同步成功！" + dataDb.getServerAddress());
            }
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
            for (int j = 0; j < 1000; j++) {
            	try {
            		Thread.sleep(10);
            	} catch (InterruptedException e) {
            		e.printStackTrace();
            	}
            	
                //通过从节点获取cs cl
            	DBCollection dbcl = null;
            	try{
            		CollectionSpace cs = dataDb.getCollectionSpace(SdbTestBase.csName);
            		dbcl = cs.getCollection(this.clName);
            	}catch(BaseException e){
            		 if ( e.getErrorCode() == -34 || e.getErrorCode() == -23) {
                         throw e;
                     } 
                     continue;
            	}
                
                //通过从节点查询
                DBCursor cursor = dbcl.query(null,null,"{\"_id\":1}",null);
                List<BSONObject> actual = new ArrayList<BSONObject>();
                while( cursor.hasNext() ) {
                    BSONObject obj = cursor.getNext();
                    actual.add(obj);
                }
                cursor.close();
                List<BSONObject> expected = new ArrayList<BSONObject>();
                //切分键[72-31)
                //期望结果[72-31)
                for( int i = 32; i <= 72; i++ ) {
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
