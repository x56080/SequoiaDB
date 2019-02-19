package com.sequoiadb.sessionaccess;

import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;

/**
* @TestLink: seqDB-14146
* @describe: 设置会话访问属性指定实例为instanceid和[M/S/A]
* @author wangkexin
* @Date   2019.02.16
* @version 1.00
*/
public class SessionAccess14146 extends SdbTestBase {
	private String clname = "cl14146";
    private Sequoiadb db;
    private DBCollection dbcl;
    private List<NodeWarrper> nodeList;
    private String rgName = "sessionAccessRG14146";

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

	@SuppressWarnings({ "rawtypes", "unchecked" })
	@Test
    public void test14146() {
    	nodeList = CommLib.getNodeList(db, rgName);
        BasicBSONList instanceidList = new BasicBSONList();
        for (NodeWarrper nodeWarrper : nodeList) {
        	instanceidList.add(nodeWarrper.getInstenceid());
        }

        //指定多个instanceid实例和"M"
        List currPreferedInstanceM = new ArrayList(instanceidList);
        currPreferedInstanceM.add("M");
        BasicBSONObject options = new BasicBSONObject("PreferedInstance", currPreferedInstanceM).append("PreferedInstanceMode", "ordered");

        db.setSessionAttr(options);
        String actualNodeName = CommLib.getActualDataNodeName(dbcl);
        NodeWarrper node = CommLib.getNodeWarrper(db, rgName, actualNodeName);
        Assert.assertTrue(node.isMaster, "testa: current node name is : " + node.getNodeName()+ "\n db.getSessionAttr():" + db.getSessionAttr());
        BasicBSONObject actual = (BasicBSONObject) db.getSessionAttr();
        Assert.assertEquals(actual, options.append("Timeout",-1L));
        
        //指定多个instanceid实例和"S"
        List currPreferedInstanceS = new ArrayList(instanceidList);
        currPreferedInstanceS.add("S");
        options = new BasicBSONObject("PreferedInstance", currPreferedInstanceS).append("PreferedInstanceMode", "ordered");
        db.setSessionAttr(options);
        actualNodeName = CommLib.getActualDataNodeName(dbcl);
        node = CommLib.getNodeWarrper(db, rgName, actualNodeName);
        
        Assert.assertFalse(node.isMaster, "testc: current node name is : " + node.getNodeName());
        actual= (BasicBSONObject) db.getSessionAttr();
        Assert.assertEquals(actual,options.append("Timeout",-1L));
        
        //指定多个instanceid实例和"m","s","a","A"
        String[] ids = new String[]{"m", "s", "a", "A"};
        for (String id : ids) {
		    List currPreferedInstance = new ArrayList(instanceidList);
		    currPreferedInstance.add(id);
		    options = new BasicBSONObject("PreferedInstance", currPreferedInstance).append("PreferedInstanceMode", "ordered");
		    db.setSessionAttr(options);
		    actualNodeName = CommLib.getActualDataNodeName(dbcl);
		    node = CommLib.getNodeWarrper(db, rgName, actualNodeName);
		    Assert.assertEquals(node.getInstenceid(), instanceidList.get(0));
		    BasicBSONObject actSessionAttr = (BasicBSONObject) db.getSessionAttr();
		    Assert.assertEquals(actSessionAttr,options.append("Timeout",-1L));
        }
    }
}
