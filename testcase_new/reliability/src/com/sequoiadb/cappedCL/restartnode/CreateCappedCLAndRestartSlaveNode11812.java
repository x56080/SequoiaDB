package com.sequoiadb.cappedCL.restartnode;

import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.Date;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.cappedCL.Utils;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.NodeRestart;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;

/**
 * @FileName seqDB-11812: creating the CappedCLs when slave node is restarted
 * @Author liuxiaoxuan
 * @Date 2017-07-31
 */

public class CreateCappedCLAndRestartSlaveNode11812 extends SdbTestBase{

	private GroupMgr groupMgr = null;
	private Sequoiadb sdb = null;
	private boolean clearFlag = false;
	private CollectionSpace cappedCS_11812 = null;
	private String cappedCSName_11812 = "story_cappedCS_restartNode_11812"; 
	private String cappedCLName_11812 = "cappedCL_restartNode_11812"; 
	private String cappedCLGroupName = null;
	private final int CAPPED_CL_NUM = 1000; 
	
	
	@BeforeClass
	public void setUp() {
        System.out.println(this.getClass().getName() + " begin at:"
                + new SimpleDateFormat("YYYY-MM-dd HH:mm:ss.SSS").format(new Date()));
        try {
			groupMgr = new GroupMgr();
			if(!groupMgr.checkBusiness()) {
	        	throw new SkipException("checkBusiness failed");
	        }
			sdb = new Sequoiadb(SdbTestBase.coordUrl,"","");
			createCappedCS();
			cappedCLGroupName = groupMgr.getAllDataGroupName().get(0);
			System.out.println("group: " + cappedCLGroupName);
		} catch (ReliabilityException e) {
			Assert.fail(this.getClass().getName() + "setUp error, error description:" + e.getMessage());
		}
        
	}
	
    @Test
    public void createCLAndRestartNodeTest() {
    	try {
    		GroupWrapper dataGroup = groupMgr.getGroupByName(cappedCLGroupName);
			NodeWrapper slaveNode = dataGroup.getSlave();
			
			FaultMakeTask faultMakeTask = NodeRestart.getFaultMakeTask(slaveNode, 1, 10);
			TaskMgr mgr = new TaskMgr(faultMakeTask);
			mgr.addTask(new createCappedCLTask());
			mgr.execute();
			//TaskMgr check if there is any exception
			Assert.assertEquals(mgr.isAllSuccess(), true, mgr.getErrorMsg());
			
			//check whether the cluster is normal and lsn consistency ,the longest waiting time is 600S
            Assert.assertEquals(groupMgr.checkBusinessWithLSN(600), true, "check LSN consistency fail");
            
            //check result
            checkCreateCLResult();
            Utils.checkConsistency(dataGroup);
            
            //Normal operating environment
            clearFlag = true;
                        
		} catch (ReliabilityException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
    }

	@AfterClass
    public void tearDown() {
    	try {
    		if(clearFlag) {
    			sdb.dropCollectionSpace(cappedCSName_11812);
    		}
    	}catch(BaseException e) {
            Assert.fail(e.getMessage());
    	}finally {
			if(sdb != null) {
				sdb.close();
				System.out.println(this.getClass().getName() + " end at:"
	                  + new SimpleDateFormat("YYYY-MM-dd HH:mm:ss.SSS").format(new Date()));
			}
		}
    }
	
	private void createCappedCS() {
		try {
			BSONObject options = new BasicBSONObject();
			options.put("Capped", true);
	        cappedCS_11812 = sdb.createCollectionSpace(cappedCSName_11812,options);
		}catch (BaseException e) {
			Assert.fail("create cappedCS failed, errMsg:" + e.getMessage());
		}   
	}

    private class createCappedCLTask extends OperateTask{

		@Override
		public void exec() throws Exception {
            try (Sequoiadb db = new Sequoiadb(SdbTestBase.coordUrl,"","")){
            	CollectionSpace cappedCS = db.getCollectionSpace(cappedCSName_11812);
            	BSONObject options = new BasicBSONObject();
            	options.put("Capped", true);
            	options.put("Size", 8192);
            	options.put("AutoIndexId", false);
            	options.put("Group", cappedCLGroupName);
            	for(int clNo = 1; clNo <= CAPPED_CL_NUM; clNo++) {
            		cappedCS.createCollection(cappedCLName_11812 + "_" + clNo , options); 
            	}
            }catch (BaseException e) {
				throw e;
			}
		} 	
    }
    
    private void checkCreateCLResult() {
        for (int clNo = 1; clNo <= CAPPED_CL_NUM; clNo++) {
            String sameCLName = cappedCLName_11812 + "_" + clNo;
            try {
                cappedCS_11812.createCollection(sameCLName);
            } catch (BaseException e) {
                // -22 SDB_DMS_EXIST
                if (-22 !=  e.getErrorCode()) {
                    throw e;
                }
            }
        }
        
    }
	
}
