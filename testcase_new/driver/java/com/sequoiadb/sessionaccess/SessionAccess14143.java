package com.sequoiadb.sessionaccess;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.text.SimpleDateFormat;
import java.util.Date;

import static org.testng.Assert.*;

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
        CommLib.createRG(db, rgName);
        BSONObject options = new BasicBSONObject("Group", rgName);
        dbcl = db.getCollectionSpace(SdbTestBase.csName).createCollection(clname, options);
    }

    @AfterClass
    public void teardown() throws InterruptedException {
    	System.out.println("the TestCase Name:" + this.getClass().getName() + 
                ". the TestCase end at:" + new SimpleDateFormat("YYYY-MM-dd HH:mm:ss.SSS").format(new Date()));
        db.getCollectionSpace(SdbTestBase.csName).dropCollection(clname);
        db.removeReplicaGroup(rgName);
        db.disconnect();
    }

    @Test
    public void test14143() {
        String[] expectPreferedInstance = new String[]{"M", "S", "A"};
        for (String s : expectPreferedInstance) {
            BasicBSONObject options = new BasicBSONObject("PreferedInstance", s);
            db.setSessionAttr(options);
            String name = CommLib.getActualDataNodeName(dbcl);
            if (s.equals("M")) {
                assertTrue(CommLib.getNodeWarrper(db, rgName, name).isMaster(), "the actual data node name is: " + name + ",the current option is " + options.toString());
            } else if (s.equals("S")) {
                assertFalse(CommLib.getNodeWarrper(db, rgName, name).isMaster(), "the actual data node name is: " + name + ",the current option is " + options.toString());
            }
            options.append("PreferedInstanceMode", "random").append("Timeout", -1L);
            assertEquals(db.getSessionAttr(),options);
        }
    }
}
