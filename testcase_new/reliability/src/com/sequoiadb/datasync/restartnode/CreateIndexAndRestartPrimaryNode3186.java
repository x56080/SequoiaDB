package com.sequoiadb.datasync.restartnode;

import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.Date;
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
import com.sequoiadb.base.DBQuery;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.datasync.Utils;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.NodeRestart;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;

/**
* FileName: CreateIndexAndRestartPrimaryNode3186.java
* test content:when create index, restart the data group master node, 
*              and the restart node is  synchronous source node.          
* testlink case:seqDB-3186
* @author wuyan
    * @Date    2017.5.2
* @version 1.00
*/


public class CreateIndexAndRestartPrimaryNode3186 extends SdbTestBase {
    private GroupMgr groupMgr = null;
    private Sequoiadb sdb = null;
    private boolean clearFlag = false;
    private String clName = "cl3186";
    private CollectionSpace cs = null;
    private static final int INDEX_NUM = 30;
    private String clGroupName = null;
    private int count = 0;

    @BeforeClass
    public void setUp() {        
        try {
            System.out.println("the TestCase Name:" + this.getClass().getName() + ". the TestCase begin at:"
                    + new SimpleDateFormat("YYYY-MM-dd HH:mm:ss.SSS").format(new Date()));
            
            groupMgr = new GroupMgr();
            if (!groupMgr.checkBusiness()) {
                throw new SkipException("checkBusiness failed");
            }

            sdb = new Sequoiadb(SdbTestBase.coordUrl, "", "");
            cs = sdb.getCollectionSpace(SdbTestBase.csName);
            clGroupName = groupMgr.getAllDataGroupName().get(0);
            createCL();
            insertData();
        } catch (ReliabilityException e) {
            Assert.fail(this.getClass().getName() + " setUp error, error description:" + e.getMessage() + "\r\n"
                    + Utils.getKeyStack(e, this));
        } 
    }

    @Test
    public void test() {        
        try {
            GroupWrapper dataGroup = groupMgr.getGroupByName(clGroupName);
            NodeWrapper priNode = dataGroup.getMaster();

            FaultMakeTask faultTask = NodeRestart.getFaultMakeTask(priNode, 4, 10, 10);
            TaskMgr mgr = new TaskMgr(faultTask);
            CreateIdxTask cTask = new CreateIdxTask();
            mgr.addTask(cTask);
            mgr.execute();
            Assert.assertEquals(mgr.isAllSuccess(), true, mgr.getErrorMsg());

            //check whether the cluster is normal and lsn consistency ,the longest waiting time is 600S
            Assert.assertEquals(groupMgr.checkBusinessWithLSN(600), true, "checkBusinessWithLSN() occurs timeout");
           
            checkCreateIndexResult();
            checkConsistency(dataGroup);
            checkExplain(dataGroup);
            
            //Normal operating environment
            clearFlag = true;
        } catch (ReliabilityException e) {
            e.printStackTrace();
            Assert.fail(e.getMessage());
        }
    }

    @AfterClass
    public void tearDown() {
        try {
        	if (clearFlag) {        		
                cs.dropCollection(clName);        		
            }            
        }catch (BaseException e) {
            Assert.fail(e.getMessage() + "\r\n" + Utils.getKeyStack(e, this));
        }finally {
        	if (sdb != null) {
        		sdb.close();
        	}
        	System.out.println(this.getClass().getName() + " end at:"
                    + new SimpleDateFormat("YYYY-MM-dd HH:mm:ss.SSS").format(new Date()));
        }
    }
    
    private void createCL() {        
        BSONObject option = (BSONObject)JSON.parse("{ Group: '" + clGroupName + "', ReplSize: 2 }");
        cs.createCollection(clName, option);
    }
    
    private void insertData() {
        DBCollection cl = cs.getCollection(clName);
        List<BSONObject> recs = new ArrayList<BSONObject>();
        int total = 10000;
        for (int i = 0; i < total; i++) {
            BSONObject rec = (BSONObject)JSON.parse("{ a" + i + ": " + i + " , b:'test'}");
            recs.add(rec);
        }
        cl.insert(recs, DBCollection.FLG_INSERT_CONTONDUP);
    }
    
    private class CreateIdxTask extends OperateTask {
        @Override
        public void exec() throws Exception {            
            try ( Sequoiadb db = new Sequoiadb(SdbTestBase.coordUrl, "", "") ){                
                DBCollection cl = db.getCollectionSpace(SdbTestBase.csName).getCollection(clName);
                for (int i = 0; i < INDEX_NUM; i++) {
                    String idxName = "idx_" + i;
                    BSONObject key = (BSONObject)JSON.parse("{ a" + i + ": 1 }");
                    boolean isUnique = i % 2 == 0 ? true : false;
                    boolean enforced = false;
                    int sortBufferSize = i * 2;
                    cl.createIndex(idxName, key, isUnique, enforced, sortBufferSize);
                    count++;
                }
            } catch (BaseException e) {            	
            	System.out.println("the create index error i is ="+count); 
            } 
        }
    }
    
    /**
	* check the result of create index,create index after recovery from failure
	*/
    private void checkCreateIndexResult() { 
    	DBCollection cl = cs.getCollection(clName);
    	//when the last index fails,which may have actually been created successfully,may fail again.    	
    	try {
    		String idxName = "idx_" + count;
    		BSONObject key = (BSONObject)JSON.parse("{ a" + count + ": 1 }");
    		cl.createIndex(idxName, key, false, false);    		    	       
        } catch (BaseException e) {            	
            // -247 Redefine index ,-46:Duplicate index name 
            if (e.getErrorCode() != -247 && e.getErrorCode() != -46 && e.getErrorCode() != -247 ) {
            	Assert.fail("the error is: "+e.getErrorCode()+e.getErrorType());
            }
        }   	
    	
    	//create remaining index 
    	try {
    		int beginNo = count +1;
    		for (int i = beginNo; i < INDEX_NUM; i++) {
    			String idxName = "idx_" + i;
        		BSONObject key = (BSONObject)JSON.parse("{ a" + i + ": 1 }");
        		cl.createIndex(idxName, key, false, false);    			
    		}    	       
        } catch (BaseException e) { 
        	Assert.fail("create index fail: "+e.getErrorCode()+e.getErrorType());            
        }     	 
    }     
    
    private void checkConsistency(GroupWrapper dataGroup) {
        boolean checkOk = false;
        int checkTimes = 30;
        int checkInterval = 1000; 
        String lastCompareInfo = "";
        for (int j = 0; j < checkTimes; j++) {
            List<String> dataUrls = dataGroup.getAllUrls();
            List<List<BSONObject>> results = new ArrayList<List<BSONObject>>();
            for (String dataUrl : dataUrls) {
                Sequoiadb dataDB = new Sequoiadb(dataUrl, "", "");
                DBCollection cl = dataDB.getCollectionSpace(SdbTestBase.csName).getCollection(clName);
                DBCursor cursor = cl.getIndexes();
                List<BSONObject> result = new ArrayList<BSONObject>();
                while (cursor.hasNext()) {
                    result.add(cursor.getNext());
                }
                results.add(result);
                cursor.close();
                dataDB.close();
            }
            
            List<BSONObject> compareA = results.get(0);
            sortByName(compareA);
            removeUnconcerned(compareA);
            checkOk = true;
            for (int i = 1; i < results.size(); i++) {
                List<BSONObject> compareB = results.get(i);
                sortByName(compareB);
                removeUnconcerned(compareB);
                if (!compareA.equals(compareB)) {
                    lastCompareInfo = "";
                    lastCompareInfo += dataUrls.get(0) + "\n";
                    lastCompareInfo += compareA + "\n";
                    lastCompareInfo += dataUrls.get(i) + "\n";
                    lastCompareInfo += compareB + "\n";
                    checkOk = false;
                }
            }
            
            if (checkOk) { break; }
            
            try {
                Thread.sleep(checkInterval);
            } catch (InterruptedException e) {
                // ignore
            }
        }
        
        if (!checkOk) {           
            Assert.fail("data is different. see the detail in console."+" lastCompareInfo="+lastCompareInfo);
        }
    }
    
    private void sortByName(List<BSONObject> list) {
        Collections.sort(list, new Comparator<BSONObject>() {
            public int compare(BSONObject a, BSONObject b) {
                String aName = (String)((BSONObject)a.get("IndexDef")).get("name");
                String bName = (String)((BSONObject)b.get("IndexDef")).get("name");
                return aName.compareTo(bName);
            }
        });
    }
    
    private void removeUnconcerned(List<BSONObject> list) {
        for (BSONObject obj : list) {
            obj.removeField("IndexFlag");
            ((BSONObject)obj.get("IndexDef")).removeField("_id");
        }
    }
    
    private void checkExplain(GroupWrapper dataGroup) {
        List<String> dataUrls = dataGroup.getAllUrls();
        for (String dataUrl : dataUrls) {
            Sequoiadb dataDB = new Sequoiadb(dataUrl, "", "");
            DBCollection cl = dataDB.getCollectionSpace(SdbTestBase.csName).getCollection(clName);
            List<String> idxNames = getIdxNames(cl);
            for (String idxName : idxNames) {
                if (!isExplainOk(cl, idxName)) {
                    Assert.fail(idxName + " does not work");
                }
            }
            dataDB.close();
        }
    }
    
    private boolean isExplainOk(DBCollection cl, String idxName) {
        BSONObject hint = (BSONObject)JSON.parse("{ '': '" + idxName + "' }");
        BSONObject run = (BSONObject)JSON.parse("{ Run: true }");
        DBCursor cursor = cl.explain(null, null, null, hint, 0, -1, DBQuery.FLG_QUERY_FORCE_HINT, run);
        BSONObject plan = cursor.getNext();
        cursor.close();
        
        if (!(plan.get("ScanType")).equals("ixscan") || 
                !(plan.get("IndexName")).equals(idxName)) {
            System.out.println("index: " + idxName);
            System.out.println("explain:" + plan);
            return false; 
        }
        return true;
    }
    
    private List<String> getIdxNames(DBCollection cl) {
        DBCursor cursor = cl.getIndexes();
        List<String> idxNames = new ArrayList<String>();
        while (cursor.hasNext()) {
            String idxName = (String)((BSONObject)cursor.getNext().get("IndexDef")).get("name");
            idxNames.add(idxName);
        }
        cursor.close();
        return idxNames;
    }
}