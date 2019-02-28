package com.sequoiadb.sessionaccess;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.bson.types.ObjectId;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import static org.testng.Assert.assertEquals;

import java.text.SimpleDateFormat;
import java.util.Date;

/**
* @TestLink: seqDB-14142
* @describe: 设置会话访问属性，指定instanceid和timeout属性
* @author wangkexin
* @Date   2019.02.16
* @version 1.00
*/

public class SessionAccess14142 extends SdbTestBase {
    private String clname = "cl14142";
    private Sequoiadb db;
    private DBCollection dbcl;
    private BasicBSONList nodes;
    private String rgName = "sessionAccessRG14142";

    @BeforeClass
    public void setup() {
    	System.out.println("the TestCase Name:" + this.getClass().getName() + 
                ". the TestCase begin at:" + new SimpleDateFormat("YYYY-MM-dd HH:mm:ss.SSS").format(new Date()));
        db = new Sequoiadb(SdbTestBase.coordUrl, "", "");
        if(CommLib.isStandAlone(db)){
			throw new SkipException("run mode is standalone,test case skip");
		}
        nodes = SessionAccessUtil.createRG(db, rgName);
        BSONObject options = new BasicBSONObject("Group", rgName);
        options.put("ReplSize", 0);
        dbcl = db.getCollectionSpace(SdbTestBase.csName).createCollection(clname, options);
        SessionAccessUtil.insertRecords(dbcl);
    }

    @Test
    public void test14142() {
    	ReplicaGroup rg = db.getReplicaGroup(rgName);
    	String slaveNodeName = rg.getSlave().getNodeName();
    	int expctId = SessionAccessUtil.getInstanceidByNodeName(nodes, slaveNodeName);
        BasicBSONObject expSessionAttr = new BasicBSONObject("PreferedInstance", expctId).append("Timeout", 1000L);
        
        //put lob 
        ObjectId oid = null;
        DBLob lob = dbcl.createLob();
        lob.write(new byte[1024 * 1024 * 10]);
        oid = lob.getID();
        lob.close();
        db.setSessionAttr(expSessionAttr);
        
        try {
        	DBLob openLob = dbcl.openLob(oid);
            openLob.close();
        } catch (BaseException e) {
        	System.out.println("catch exception!");
        	Assert.assertEquals(e.getErrorCode(), -13);
        }
        
        expSessionAttr.put("Timeout", 20000L);
        db.setSessionAttr(expSessionAttr);
        DBLob openLob = dbcl.openLob(oid);
        openLob.close();
        
        String actualNodeName = SessionAccessUtil.getActualDataNodeName(dbcl);
        assertEquals(slaveNodeName, actualNodeName);
        BSONObject actSessionAttr = db.getSessionAttr();
        expSessionAttr.append("PreferedInstanceMode", "random");
        expSessionAttr.append("Timeout", 20000L);
        assertEquals(actSessionAttr, expSessionAttr);
    }
    
    @AfterClass
    public void teardown() throws InterruptedException {
    	System.out.println("the TestCase Name:" + this.getClass().getName() + 
                ". the TestCase end at:" + new SimpleDateFormat("YYYY-MM-dd HH:mm:ss.SSS").format(new Date()));
        CollectionSpace currCS = db.getCollectionSpace(SdbTestBase.csName);
        currCS.dropCollection(clname);
        db.removeReplicaGroup(rgName);
        db.disconnect();
    }
}
