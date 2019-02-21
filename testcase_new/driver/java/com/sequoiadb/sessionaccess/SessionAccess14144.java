package com.sequoiadb.sessionaccess;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.text.SimpleDateFormat;
import java.util.Date;

import static org.testng.Assert.assertEquals;

/**
* @TestLink: seqDB-14144
* @describe: 设置会话访问属性指定多个instanceid，其中节点选择模式为顺序选取
* @author wangkexin
* @Date   2019.02.16
* @version 1.00
*/
public class SessionAccess14144 extends SdbTestBase {
	private String clname = "cl14144";
    private Sequoiadb db;
    private DBCollection dbcl;
    private BasicBSONList nodes;
    private String rgName = "sessionAccessRG14144";

    @BeforeClass
    public void setup() {
    	System.out.println("the TestCase Name:" + this.getClass().getName() + 
                ". the TestCase begin at:" + new SimpleDateFormat("YYYY-MM-dd HH:mm:ss.SSS").format(new Date()));
        db = new Sequoiadb(SdbTestBase.coordUrl, "", "");
        nodes = CommLib.createRG(db, rgName);
        BSONObject options = new BasicBSONObject("Group", rgName);
        options.put("ReplSize", -1);
        dbcl = db.getCollectionSpace(SdbTestBase.csName).createCollection(clname, options);
        CommLib.insertRecords(dbcl);
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
    public void test14144() {
        BasicBSONList instanceidList = CommLib.getInstanceidList(nodes);

        BasicBSONObject options = new BasicBSONObject("PreferedInstance", instanceidList).append("PreferedInstanceMode", "ordered");
        db.setSessionAttr(options);
        String actualNodeName = CommLib.getActualDataNodeName(dbcl);
        
        BasicBSONObject expNode = (BasicBSONObject)nodes.get(0);
        assertEquals(actualNodeName, expNode.getString("nodeName"));
        //assert getSessionAttr
        BSONObject actual = db.getSessionAttr();
        options.append("Timeout",-1L);
        assertEquals(actual,options);
    }

}
