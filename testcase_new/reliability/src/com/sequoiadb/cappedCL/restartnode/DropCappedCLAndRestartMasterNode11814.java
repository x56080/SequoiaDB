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
 * @FileName seqDB-11814: droping the CappedCLs when primary node is restarted
 * @Author liuxiaoxuan
 * @Date 2017-07-31
 */

public class DropCappedCLAndRestartMasterNode11814 extends SdbTestBase{
	
	private GroupMgr groupMgr = null;
	private Sequoiadb sdb = null;
	private boolean clearFlag = false;
	private CollectionSpace cappedCS_11814 = null;
	private String cappedCSName_11814 = "story_cappedCS_restartNode_11814"; 
	private String cappedCLName_11814 = "cappedCL_restartNode_11814"; 
	private String cappedCLGroupName = null;
	private final int CAPPED_CL_NUM = 1000; 
	private int dropCLCounts = 0;
	
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
			
			cappedCLGroupName = groupMgr.getAllDataGroupName().get(0);
			createCLs();
		} catch (ReliabilityException e) {
			Assert.fail(this.getClass().getName() + "setUp error, error description:" + e.getMessage());
		}
        
	}
	
	@Test
	public void dropCLAndRestartNodeTest() {
		try {
    		GroupWrapper dataGroup = groupMgr.getGroupByName(cappedCLGroupName);
			NodeWrapper primaryNode = dataGroup.getMaster();
			
			FaultMakeTask faultMakeTask = NodeRestart.getFaultMakeTask(primaryNode, 1, 10);
			TaskMgr mgr = new TaskMgr(faultMakeTask);
			mgr.addTask(new DropCappedCLTask());
			mgr.execute();
			//TaskMgr check if there is any exception
			Assert.assertEquals(mgr.isAllSuccess(), true, mgr.getErrorMsg());
			
			//check whether the cluster is normal and lsn consistency ,the longest waiting time is 600S
            Assert.assertEquals(groupMgr.checkBusinessWithLSN(600), true, "check LSN consistency fail");
            
            //check result
            checkDropCappedCLResult();
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
    			sdb.dropCollectionSpace(cappedCSName_11814);
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
	
	 private void createCLs() {
		 try {
			BSONObject cs_option = new BasicBSONObject();
	        cs_option.put("Capped", true);
		    cappedCS_11814 = sdb.createCollectionSpace(cappedCSName_11814,cs_option);
		    BSONObject cl_option = new BasicBSONObject();
		    cl_option.put("Capped", true);
		    cl_option.put("Size", 8192);
		    cl_option.put("AutoIndexId", false);
		    cl_option.put("Group", cappedCLGroupName);
			for (int clNo = 1; clNo <= CAPPED_CL_NUM; clNo++) {
			    cappedCS_11814.createCollection(cappedCLName_11814 + "_" + clNo , cl_option);
			}
	     }catch (BaseException e) {
			Assert.fail("create failed, errMsg:" + e.getMessage());
		 }   	
	 }
	    
	 private class DropCappedCLTask extends OperateTask {
	        @Override
	        public void exec() throws Exception {
	            Sequoiadb db = null;
	            try {
	                db = new Sequoiadb(coordUrl, "", "");
	                CollectionSpace cappedCS = db.getCollectionSpace(cappedCSName_11814);
	                for (int clNo = 1; clNo <= CAPPED_CL_NUM; clNo++) {
	                    String clName = cappedCLName_11814 + "_" + clNo;
	                    cappedCS.dropCollection(clName);
	                    ++dropCLCounts;
	                }
	            } catch (BaseException e) {
	            	System.out.println("the drop CL num is = " + dropCLCounts);
	            } finally {
	                if (db != null) {
	                    db.close();
	                }
	            }
	        }
	  }
	
	 private void checkDropCappedCLResult() {
	        if (CAPPED_CL_NUM == dropCLCounts) {        	
	        	try {
	        		String sameCLName = cappedCLName_11814 + "_" + dropCLCounts;
	                cappedCS_11814.dropCollection(sameCLName);               
	                Assert.fail("drop the same CL failed");                
	            } catch (BaseException e) {
	                // -23 Collection does not exist  
	                if (-23 !=  e.getErrorCode()) {
	                	Assert.fail("the error not -23: " + e.getErrorCode());
	                }
	            }         	
	        }else{
	        	//drop remain CLs
	        	for(int clNo = dropCLCounts + 1; clNo <= CAPPED_CL_NUM; clNo++){
	        		String sameCLName = cappedCLName_11814 + "_" + clNo;
	            	try {         	
	            		cappedCS_11814.dropCollection(sameCLName);            		
	                } catch (BaseException e) {                 	
	                	Assert.fail("the error: " + e.getErrorCode() + " " + e.getMessage());
	                }        		
	        	}
	        } 
	 } 
	 
}
