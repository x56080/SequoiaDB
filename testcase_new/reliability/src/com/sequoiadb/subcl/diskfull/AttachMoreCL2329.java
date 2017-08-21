package com.sequoiadb.subcl.diskfull;

import java.text.SimpleDateFormat;
import java.util.Date;

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
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.DiskFull;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;

/**
 * @FileName seqDB-2329: detachCL过程中catalog主节点所在服务器磁盘满_rlb.diskExhaustion.subCL.004
 * @Author liuxiaoxuan
 * @Date 2017-08-18
 * @Version 1.00
 */

/*
 * 1、创建主表和子表（如循环创建50个主表，每个主表挂载500个子表），CL数组节点为dataRG主节点
 * 2、批量执行db.collectionspace.collection.detachCL()分离多个子表
 * 3、子表分离过程中模拟dataRG主节点磁盘满，检查detachCL执行结果 
 * 4、将dataRG主节点故障恢复，并对分离成功的子表（普通表）做基本操作（如insert)
 */
public class AttachMoreCL2329 extends SdbTestBase{

	private Sequoiadb sdb = null;
	private GroupMgr groupMgr = null;
	private boolean clearFlag = false;
	private String csName = "cs_2329";
    private String mainCLName = "maincl_2329";
    private String subCLName = "subcl_2329";
    private String clGroupName = null;
    private CollectionSpace cs = null;
    private final int MAINCL_NUMS = 50;
    private final int SUBCL_NUMS = 100;
    private int successDetachNums = 0;

    @BeforeClass
    public void setUp() {
       
        try {
            System.out.println("the TestCase Name:" + this.getClass().getName() + ". the TestCase begin at:"
                    + new SimpleDateFormat("YYYY-MM-dd HH:mm:ss.SSS").format(new Date()));

            groupMgr = new GroupMgr();
            if (!groupMgr.checkBusiness()) {
                throw new SkipException("checkBusiness failed");
            }

            clGroupName = groupMgr.getAllDataGroupName().get(0);
            sdb = new Sequoiadb(SdbTestBase.coordUrl, "", "");
            createAndAttachCLs();
        } catch (ReliabilityException e) {
            Assert.fail(this.getClass().getName() + " setUp error, error description:" + e.getMessage());
        } 
    }
    
    @AfterClass
    public void tearDown() {
    	try {
    		if(clearFlag) {
    			sdb.dropCollectionSpace(csName);
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

    @Test
    public void test() {
        try {
        	GroupWrapper dataGroup = groupMgr.getGroupByName(clGroupName);
			NodeWrapper primaryNode = dataGroup.getMaster();
			
            FaultMakeTask faultTask = DiskFull.getFaultMakeTask(primaryNode.hostName(),
                    SdbTestBase.reservedDir, 1, 10);
            TaskMgr mgr = new TaskMgr(faultTask);
            DetachCLTask detachCLTask = new DetachCLTask();
            mgr.addTask(detachCLTask);
            mgr.execute();
            Assert.assertEquals(mgr.isAllSuccess(), true, mgr.getErrorMsg());

            //check whether the cluster is normal and lsn consistency ,the longest waiting time is 600S
            Assert.assertEquals(groupMgr.checkBusinessWithLSN(600), true, "check LSN consistency fail");
            Assert.assertEquals(dataGroup.checkInspect(1), true, "data is different on " + dataGroup.getGroupName());

            checkDetachResult();
            checkInsert();
            clearFlag = true;
        } catch (ReliabilityException e) {
            e.printStackTrace();
            Assert.fail(e.getMessage());
        } 
    }
    
    public void createAndAttachCLs() {
    	try {
    		cs = sdb.createCollectionSpace(csName);
			for(int i = 0; i < MAINCL_NUMS; i++) {
				DBCollection mainCL = cs.createCollection(mainCLName + "_" + i,
		                    (BSONObject) JSON
		                            .parse("{ShardingKey:{'key':1},ShardingType:'range',IsMainCL:true}"));   
				for(int j = 0; j <SUBCL_NUMS; j++) {
					BSONObject subclOption = new BasicBSONObject();
					subclOption.put("Group", clGroupName);
					cs.createCollection(subCLName + "_" + i + "_" + j , subclOption);
					String sclFullName = csName + "." + subCLName + "_" + i + "_" + j ;
					mainCL.attachCollection(sclFullName, (BSONObject) JSON.parse("{ LowBound: { key: " + ((i + j) * 100)
	                        + " }, " + "UpBound: { key: " + ((i + j + 1) * 100) + " } }"));
				}
			}
		}catch (BaseException e) {
			Assert.fail("CreateAndAttach cl failed, errMsg:" + e.getMessage());
		}   
    }
    
    private class DetachCLTask extends OperateTask{

		@Override
		public void exec() throws Exception {
			 Sequoiadb db = null;
		        try {
		            db = new Sequoiadb(SdbTestBase.coordUrl, "", "");
		            CollectionSpace newCS = db.getCollectionSpace(csName);  
		            for (int i = 0; i < MAINCL_NUMS; i++) {
		                 DBCollection mainCL = newCS.getCollection(mainCLName + "_" + i);
		            	 for (int j = 0; j < SUBCL_NUMS; j++) {
				            String sclFullName = csName + "." + subCLName + "_" + i + "_" + j;
				            mainCL.detachCollection(sclFullName);
				            ++successDetachNums;
				         }
		            }   
		        } catch (BaseException e) {
		        	System.out.println("success detach cl num is = " + successDetachNums);
		        } finally {	
		            if (db != null) {
		                db.close();
		            }
		        }
		} 	
    	
    }
    
    public void checkDetachResult() {
    	int f = 0;
    	for (int i = 0; i < MAINCL_NUMS; i++) {
    		DBCollection mcl = cs.getCollection(mainCLName + "_" + i);
      	    for (int j = 0; j < SUBCL_NUMS; j++) {
      		   try {
      			  if(f > successDetachNums || 0 == successDetachNums) {
      				  break;
      			  }
      			  String sclFullName = csName + "." + subCLName + "_" + i + "_" + j;
		          mcl.detachCollection(sclFullName);
      			  f++;
      		  } catch (BaseException e) {
      			  
      			Assert.assertEquals(e.getErrorCode(),-242,"the error code is not -242: " + e.getErrorCode());
	      	  } 
      	  } 
      }
    }
    
    public void checkInsert() {
    	int detachMainCLNums = successDetachNums / SUBCL_NUMS;
    	for (int i = 0; i < detachMainCLNums; i++) {
      	    for (int j = 0; j < SUBCL_NUMS; j++) {
      		   try {
      			  String sclFullName = subCLName + "_" + i + "_" + j;
		          DBCollection detachedCL = cs.getCollection(sclFullName);
		          int insertNums = 1000;
		          for(int num = 0; num < insertNums; num++) {
		        	  BSONObject insrtObj = new BasicBSONObject();
		        	  insrtObj.put("key", "testaaaaaaaaaaaaaaaaaaaaaaaaaa" + i * j * insertNums);
		        	  detachedCL.insert(insrtObj);
		          }
      		  } catch (BaseException e) {
      			Assert.fail("insert failed: " + e.getMessage());
	      	  } 
      	  } 
      }
    }
   
}
