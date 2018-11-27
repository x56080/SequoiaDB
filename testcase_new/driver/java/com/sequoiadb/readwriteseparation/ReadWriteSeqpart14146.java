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

import java.util.ArrayList;
import java.util.List;
import java.util.Random;

import static com.sequoiadb.readwriteseparation.Helper.getActualDataNodeName;
import static com.sequoiadb.readwriteseparation.Helper.getNodeList;
import static org.testng.Assert.assertEquals;

/**
 * Created by laojingtang on 18-1-19.
 * Modified by wangkexin on 18-11-27.
 */
public class ReadWriteSeqpart14146 extends SdbTestBase {
    private final String CLNAME = this.getClass().getSimpleName();
    private Sequoiadb db;
    private DBCollection dbcl;
    private Random random = new Random();
    private List<NodeWarrper> nodeList;
    private String rgName = Const.RGNAME + "14146";

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


    private int getRandomInstanceid() {
        return nodeList.get(random.nextInt(nodeList.size())).getInstenceid();
    }

    private NodeWarrper getNodeWarrper(String nodename) {
        for (NodeWarrper nodeWarrper : nodeList) {
            if (nodeWarrper.getNodeName().equals(nodename)) {
                return nodeWarrper;
            }
        }
        return null;
    }

    private boolean isMaster(String nodename) {
        return getNodeWarrper(nodename).isMaster();
    }


    /**
     * 1、连接coord执行db.setSessionAttr()，设置PreferedInstance会话实例为instanceid和【M/S/A】,访问模式为顺序模式 ，分别验证如下场景：
     * 1）、指定多个instanceid实例和“M”、“m”
     * 2）、指定多个instanceid实例和“S”(或“s”)
     * 3）、指定多个instanceid实例和“A”或者“a”
     * 4）、指定多个instanceid实例和“M/S/A"
     * 3、连接该coord节点，执行查询操作 4、查看访问节点情况（如执行explain查看）
     * 查看访问结果如下：
     * 1）、存在M，查看访问连接节点为instanceid对应的主节点；存在m，查看访问节点为指定第一个instanceid对应节点
     * 2）、存在S，查看访问连接节点为instanceid对应的备节点；存在"s"，则访问节点为第一个instanceid对应节点（如instanceid为主节点则访问主节点）
     * 3）、查看访问连接节点在指定第一个instanceid对应的节点
     * 4）、查看访问连接节点为主节点
     */
    @Test(invocationCount = 1)
    public void test14146() {
        BasicBSONList idList = new BasicBSONList();
        for (NodeWarrper nodeWarrper : nodeList) {
            idList.add(nodeWarrper.getInstenceid());
        }

        char[] ids = new char[]{'m', 'M', 'S', 's', 'a', 'A'};
        for (char id : ids) {
            List l = new ArrayList(idList);
            l.add(id);
            BasicBSONObject options = new BasicBSONObject("PreferedInstance", idList).append("PreferedInstanceMode", "ordered");
            db.setSessionAttr(options);
            String actualNodeName = getActualDataNodeName(dbcl);
            NodeWarrper node = getNodeWarrper(actualNodeName);

            assertEquals(node.getInstenceid(), idList.get(0));

            BasicBSONObject actual= (BasicBSONObject) db.getSessionAttr();
            assertEquals(actual,options.append("Timeout",-1L));
        }
    }

}
