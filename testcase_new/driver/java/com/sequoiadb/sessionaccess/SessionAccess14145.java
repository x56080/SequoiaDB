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
import java.util.ArrayList;
import java.util.Date;
import java.util.List;

import static org.testng.Assert.assertEquals;
import static org.testng.Assert.fail;

/**
* @TestLink: seqDB-14145
* @describe: 设置会话访问属性指定多个instanceid，其中节点选择模式为随机选取
* @author wangkexin
* @Date   2019.02.16
* @version 1.00
*/
public class SessionAccess14145 extends SdbTestBase {
	private String clname = "cl14145";
    private Sequoiadb db;
    private DBCollection dbcl;
    private BasicBSONList nodes;
    private String rgName = "sessionAccessRG14145";

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
    public void test14145() {
    	List<Integer> instanceidList = new ArrayList<Integer>();
        for (int i=0 ; i< nodes.size() ; i++) {
        	BasicBSONObject node = (BasicBSONObject)nodes.get(i);
        	instanceidList.add(Integer.parseInt(node.getString("instanceid")));
        }
        
    	int[] id = new int[]{instanceidList.get(0), instanceidList.get(1)};

        BSONObject options = new BasicBSONObject("PreferedInstance", id).append("PreferedInstanceMode", "random");
        db.setSessionAttr(options);
        String actualNodeName = CommLib.getActualDataNodeName(dbcl);
        int actualId = CommLib.getInstanceidByNodeName(nodes, actualNodeName);
        if (actualId != id[0] && actualId != id[1]) {
            fail("actual:" + actualId + " expect: " + id[0] + " or " + id[1]);
        }

        BSONObject actual = db.getSessionAttr();
        BasicBSONList actualIdList= (BasicBSONList) actual.get("PreferedInstance");
        BasicBSONList expect=new BasicBSONList();
        for (int i : id) {
            expect.add(i);
        }
        assertEquals(actualIdList,expect);

        assertEquals(actual.get("PreferedInstanceMode"),"random");
    }
}
