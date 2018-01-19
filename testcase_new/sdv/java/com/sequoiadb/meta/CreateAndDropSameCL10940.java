package com.sequoiadb.meta;

import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
* FileName: CreateAndDropCL10940.java
* test content:concurrent creation and deletion of same cl
* testlink case:seqDB-10940
* @author wuyan
    * @Date    2016.9.12
* @version 1.00
*/
public class CreateAndDropSameCL10940 extends SdbTestBase {	
	
	private String clName = "cl10940";
	private static Sequoiadb sdb = null;
	String clGroupName = null;
	
	@BeforeClass
	public void setUp( ){
		try{
			sdb = new Sequoiadb(SdbTestBase.coordUrl, "sdbadmin", "sdbadmin");
		}catch(BaseException e){			
			Assert.assertTrue(false,"connect %s failed,"+SdbTestBase.coordUrl+e.getMessage());
		}		

	}
	
	@Test
	public void createAndDropCL10940(){
		CreateCLThread createCLThread = new CreateCLThread();
		DropCLThread dropCLThread = new DropCLThread();		
		
		createCLThread.start();
		dropCLThread.start();
		
		 if(!(createCLThread.isSuccess() && dropCLThread.isSuccess())){
	            List<Exception> exceptions = new ArrayList<Exception>();
	            exceptions.addAll(createCLThread.getExceptions());
	            exceptions.addAll(dropCLThread.getExceptions());
	            
	            String errMsg = "";
	            for(int i = 0; i < exceptions.size(); i++){
	                exceptions.get(i).printStackTrace(); 
	                errMsg += exceptions.get(i).getMessage() + "\n";
	            }
	            Assert.fail(errMsg);
	        }		
	}
	
	@AfterClass(alwaysRun = true)
	public void tearDown(){
		try{
			CollectionSpace cs = sdb.getCollectionSpace(SdbTestBase.csName);
			if(cs.isCollectionExist(clName)){
				cs.dropCollection(clName);
			}			
			sdb.disconnect();
		}catch(BaseException e){			
			Assert.assertTrue(false,"clean up failed:"+e.getMessage());
		}
	}	

	
	class CreateCLThread extends SdbThreadBase {
        @Override
        public void exec() throws BaseException{
            Sequoiadb db1 = null;
            CollectionSpace cs1 = null;
            DBCollection dbcl = null;
            
            try{
                db1 = new Sequoiadb(SdbTestBase.coordUrl, "sdbadmin", "sdbadmin");
                cs1 = db1.getCollectionSpace(SdbTestBase.csName);
                dbcl = cs1.createCollection(clName);     
                checkCreateCl(dbcl);
            }catch(BaseException e){
            	Assert.assertEquals(-23,e.getErrorCode(),e.getMessage());
            }finally{
            	if(db1 != null){
            		db1.disconnect();            		
            	}                
            }
        }
    }
	
	class DropCLThread extends SdbThreadBase {
        @Override
        public void exec() throws BaseException{
            Sequoiadb db2 = null;
            CollectionSpace cs2 = null;     
            try{
                db2 = new Sequoiadb(SdbTestBase.coordUrl, "sdbadmin", "sdbadmin");
                cs2 = db2.getCollectionSpace(SdbTestBase.csName);    
                cs2.dropCollection(clName);   
                Assert.assertFalse(cs2.isCollectionExist(clName));
            }catch(BaseException e){
            	Assert.assertEquals(-23,e.getErrorCode(),"drop cl fail "+e.getMessage());
            }finally{
            	if(db2 != null){
            		db2.disconnect();            		
            	}                
            }
        }
    }
		
	public void checkCreateCl(DBCollection cl){
		try{
			cl.insert("{a:1}");	
			Assert.assertEquals(cl.getCount(), 1);
		}catch(BaseException e){
			if (e.getErrorCode() != -23) {
				Assert.fail("insert fail, " + e.getMessage());
			}
		}
	}
}
