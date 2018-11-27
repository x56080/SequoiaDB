package com.sequoiadb.readwriteseparation;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.List;

import static com.sequoiadb.readwriteseparation.Helper.getActualDataNodeName;
import static com.sequoiadb.readwriteseparation.Helper.getNodeList;
import static org.testng.Assert.assertEquals;

/**
 * Created by laojingtang on 18-1-19.
 * Modified by wangkexin on 18-11-27.
 */
public class ReadWriteSeqpart14144 extends SdbTestBase {
    private final String CLNAME = this.getClass().getSimpleName();
    private Sequoiadb db;
    private DBCollection dbcl;
    private List<NodeWarrper> nodeList;
    private String rgName = Const.RGNAME + "14144";

    @BeforeClass
    public void setup() {
        db = new Sequoiadb(SdbTestBase.coordUrl, "", "");
        CommLib.createRG(db, rgName);
        BSONObject options = new BasicBSONObject("Group", rgName);
        dbcl = db.getCollectionSpace(SdbTestBase.csName).createCollection(CLNAME, options);
        nodeList = getNodeList(db, rgName);
    }

    @AfterClass
    public void teardown() throws InterruptedException {
        db.getCollectionSpace(SdbTestBase.csName).dropCollection(CLNAME);
        db.removeReplicaGroup(rgName);
        db.disconnect();
    }

    /**
     * 1、连接coord执行db.setSessionAttr()，设置PreferedInstance会话实例为多个instanceid（如节点b、a对应instanceid），访问模式为ordered
     * 2、执行查询操作（如查询lob）
     * 4、查看访问节点信息（如执行explain查看）
     */
    @Test(invocationCount = 1)
    public void test14144() {
        BasicBSONList idList = new BasicBSONList();
        for (NodeWarrper nodeWarrper : nodeList) {
            idList.add(nodeWarrper.getInstenceid());
        }

        BasicBSONObject options = new BasicBSONObject("PreferedInstance", idList).append("PreferedInstanceMode", "ordered");
        db.setSessionAttr(options);
        String actualNodeName = getActualDataNodeName(dbcl);
        assertEquals(actualNodeName, nodeList.get(0).getNodeName());
        //assert getSessionAttr
        BSONObject actual = db.getSessionAttr();
        options.append("Timeout",-1L);
        assertEquals(actual,options);
    }

}
