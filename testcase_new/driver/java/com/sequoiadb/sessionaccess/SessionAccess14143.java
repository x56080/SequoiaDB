package com.sequoiadb.sessionaccess;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import static org.testng.Assert.*;

import java.text.SimpleDateFormat;
import java.util.Date;

/**
* @TestLink: seqDB-14143
* @describe: 设置会话访问属性，单值指定访问实例为M/S/A
* @author wangkexin
* @Date   2019.02.16
* @version 1.00
*/
public class SessionAccess14143 extends SdbTestBase {
	private String clname = "cl14143";
    private Sequoiadb db;
    private DBCollection dbcl;
    private String rgName = "sessionAccessRG14143";

    @BeforeClass
    public void setup() {
    	System.out.println("the TestCase Name:" + this.getClass().getName() + 
                ". the TestCase begin at:" + new SimpleDateFormat("YYYY-MM-dd HH:mm:ss.SSS").format(new Date()));
        db = new Sequoiadb(SdbTestBase.coordUrl, "", "");
        if(com.sequoiadb.testcommon.CommLib.isStandAlone(db)){
			throw new SkipException("run mode is standalone,test case skip");
		}
        CommLib.createRG(db, rgName);
        BSONObject options = new BasicBSONObject("Group", rgName);
        options.put("ReplSize", -1);
        dbcl = db.getCollectionSpace(SdbTestBase.csName).createCollection(clname, options);
        CommLib.insertRecords(dbcl);
    }

    @Test
    public void test14143() {
    	int isMasterNum = 0;
        String[] expectPreferedInstance = new String[]{"M", "S", "A"};
        for(int i = 0 ; i < 20 ; i++){
	        for (String s : expectPreferedInstance) {
	            BasicBSONObject options = new BasicBSONObject("PreferedInstance", s);
	            db.setSessionAttr(options);
	            String hostName = CommLib.getActualDataNodeName(dbcl);
	            if (s.equals("M")) {
	                assertTrue(CommLib.isMaster(db, rgName, hostName), "the actual data node name is: " + hostName + ",the current option is " + options.toString());
	            } else if (s.equals("S")) {
	                assertFalse(CommLib.isMaster(db, rgName, hostName), "the actual data node name is: " + hostName + ",the current option is " + options.toString());
	            }else if (s.equals("A")) {
	                if(CommLib.isMaster(db, rgName, hostName)){
	                	isMasterNum++;
	                }
	            }
	            options.append("PreferedInstanceMode", "random").append("Timeout", -1L);
	            assertEquals(db.getSessionAttr(),options);
	        }
        }
        if(isMasterNum == 20){
        	Assert.fail("set PreferedInstance is 'A' , actual data node has always been the master node");
        }else if(isMasterNum == 0){
        	Assert.fail("set PreferedInstance is 'A' , actual data node has always been the spare node");
        }
    }
    
    @AfterClass
    public void teardown() throws InterruptedException {
    	System.out.println("the TestCase Name:" + this.getClass().getName() + 
                ". the TestCase end at:" + new SimpleDateFormat("YYYY-MM-dd HH:mm:ss.SSS").format(new Date()));
        db.getCollectionSpace(SdbTestBase.csName).dropCollection(clname);
        db.removeReplicaGroup(rgName);
        db.disconnect();
    }
}
