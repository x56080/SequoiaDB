package com.sequoiadb.subcl.brokennetwork;

import java.text.SimpleDateFormat;
import java.util.Date;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.CommLib;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.BrokenNetwork;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;

/**
 * @FileName seqDB-2181: 在主表insert大量数据时catalog主节点断网_rlb.netSplit.subCL.013 
 * @Author liuxiaoxuan
 * @Date 2017-08-18
 * @Version 1.00
 */

/*
 * 1、创建主表和子表
 * 2、在主表插入大量数据（如每个子表插入10万条数据），插入数据过程中将catalog主节点网络断掉
 * 3、将catalog主节点网络恢复，检查dataRG各节点数据是否完整一致； 
 * 4、对原操作的主表重新插入数据，检查返回结果 
 */
public class InsertAndCatalogBroken2181 extends SdbTestBase{
	
	private GroupMgr groupMgr = null;
    private boolean clearFlag = false;
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private String csName = "cs_2181";
    private String mainCLName = "maincl_2181";
    private String subCLName = "subcl_2181";
    private String connectUrl = null;
    private String srcGroupName = null;
	private String destGroupName = null;
    private final int SUBCL_NUMS = 5;
    private int successInsertNums = 0;
    private final int INSERT_NUMS = 100000;

    @BeforeClass
    public void setUp() {
        
        try {
            System.out.println("the TestCase Name:" + this.getClass().getName() + ". the TestCase begin at:"
                    + new SimpleDateFormat("YYYY-MM-dd HH:mm:ss.SSS").format(new Date()));

            groupMgr = new GroupMgr();
            if (!groupMgr.checkBusiness()) {
                throw new SkipException("checkBusiness failed");
            }
            srcGroupName = groupMgr.getAllDataGroupName().get(0);
			destGroupName =groupMgr.getAllDataGroupName().get(1);
            sdb = new Sequoiadb(SdbTestBase.coordUrl,"","");
            
            createAndAttachCLs();
        } catch (ReliabilityException e) {
            Assert.fail(this.getClass().getName() + " setUp error, error description:" + e.getMessage());
        } 
    }

    @AfterClass
    public void tearDown() {
    	Sequoiadb db = null;
    	try {
    		db = new Sequoiadb(SdbTestBase.coordUrl,"","");
    		if(clearFlag) {
    			db.dropCollectionSpace(csName);
    		}
    	}catch(BaseException e) {
            Assert.fail(e.getMessage());
    	}finally {
			if(db != null) {
				db.close();
				System.out.println(this.getClass().getName() + " end at:"
	                  + new SimpleDateFormat("YYYY-MM-dd HH:mm:ss.SSS").format(new Date()));
			}
		}
    }
    
    @Test
    public void test() {
        
        try {
        	GroupWrapper cataGroup = groupMgr.getGroupByName("SYSCatalogGroup");
			NodeWrapper primaryNode = cataGroup.getMaster();
			connectUrl = CommLib.getSafeCoordUrl(primaryNode.hostName());
			
            FaultMakeTask faultTask = BrokenNetwork.getFaultMakeTask(primaryNode.hostName(), 5, 10);
            TaskMgr mgr = new TaskMgr(faultTask);
            InsertTask insertTask = new InsertTask();
            mgr.addTask(insertTask);
            mgr.execute();
            Assert.assertEquals(mgr.isAllSuccess(), true, mgr.getErrorMsg());

            //check whether the cluster is normal and lsn consistency ,the longest waiting time is 600S
            Assert.assertEquals(groupMgr.checkBusinessWithLSN(600), true, "check LSN consistency fail");
            
            splitSubCLs();
            
            GroupWrapper srcdataGroup = groupMgr.getGroupByName(srcGroupName);
            GroupWrapper destdataGroup = groupMgr.getGroupByName(destGroupName);
            Assert.assertEquals(srcdataGroup.checkInspect(1), true, "data is different on " + srcdataGroup.getGroupName());
            Assert.assertEquals(destdataGroup.checkInspect(1), true, "data is different on " + destdataGroup.getGroupName());
           
            checkInsertResult();
            
            clearFlag = true;
        } catch (ReliabilityException e) {
            e.printStackTrace();
            Assert.fail(e.getMessage());
        } 
    }

    public void createAndAttachCLs() {
    	try {
    		cs = sdb.createCollectionSpace(csName);
    		DBCollection mainCL = cs.createCollection(mainCLName,
                    (BSONObject) JSON
                            .parse("{ShardingKey:{'a':1},ShardingType:'range',IsMainCL:true}"));   
		for(int i = 0; i < SUBCL_NUMS; i++) {
			cs.createCollection(subCLName + "_" + i,
					(BSONObject) JSON
                    .parse("{ShardingKey:{b:1},ShardingType:'range',"
        					+ "Group:'" + srcGroupName + "'}"));
			String sclFullName = csName + "." + subCLName + "_" + i;
			mainCL.attachCollection(sclFullName, (BSONObject) JSON.parse("{ LowBound: { a: " + i * 100000
                    + " }, " + "UpBound: { a: " + ((i + 1) * 100000) + " } }"));
		}
		}catch (BaseException e) {
			Assert.fail("CreateAndAttach cl failed, errMsg:" + e.getMessage());
		}   
    }
    
    class InsertTask extends OperateTask {
    	
        @Override
        public void exec() throws Exception {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb(connectUrl, "", "");
                CollectionSpace newCS = db.getCollectionSpace(csName);
                DBCollection mainCL = newCS.getCollection(mainCLName);
                for (int i = 0; i < INSERT_NUMS * SUBCL_NUMS; i++) {
                	mainCL.insert("{ a: " + i +  ", b: 'test" + i + "'}");
                    ++successInsertNums;
                }
            } catch (BaseException e) {
            	System.out.println("success insert num is = " + successInsertNums);
            } finally {
                if (db != null) {
                    db.close();
                }
            }
        }

       
    }
    
    public void splitSubCLs() {
    	try {
            for (int i = 0; i < SUBCL_NUMS; i++) {
            	DBCollection subCL = sdb.getCollectionSpace(csName).getCollection(subCLName + "_" + i);
            	subCL.split(srcGroupName, destGroupName, 50);
            }
    	}catch(BaseException e) {
            Assert.fail("split subcl failed: " + e.getMessage());
    	}
    }
    
    public void checkInsertResult(){
        try {
            DBCollection mainCL = sdb.getCollectionSpace(csName).getCollection(mainCLName);
            for (int i = 0; i < SUBCL_NUMS; i++) {
                int lowBound = i * INSERT_NUMS;
                int upBound = (i + 1) * INSERT_NUMS - 1;
                mainCL.insert("{ a: " + lowBound +  ", b: 'test" + i + "'" + ", c: " + i + "}");
                mainCL.insert("{ a: " + upBound +  ", b: 'test" + i + "'" + ", c: " + i + "}");
                if (2 != mainCL.getCount("{ c: " + i + " }")) {
                    Assert.fail("subcl_" + i + " is disabled");
                }
            }
        } catch (BaseException e) {
        	 Assert.fail("check insert again failed: " + e.getMessage());
        }
    }

}
