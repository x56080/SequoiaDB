package com.sequoiadb.readwriteseparation;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.List;
import java.util.Random;

import static com.sequoiadb.readwriteseparation.Helper.getActualDataNodeName;
import static com.sequoiadb.readwriteseparation.Helper.getNodeList;
import static org.testng.Assert.*;

/**
 * Created by laojingtang on 18-1-19.
 */
public class ReadWriteSeqpart14143 extends SdbTestBase {
    private final String CLNAME = this.getClass().getSimpleName();
    private Sequoiadb db;
    private DBCollection dbcl;
    private Random random = new Random();
    private List<NodeWarrper> nodeList;

    @BeforeClass
    public void setup() {
        db = new Sequoiadb(super.coordUrl, "", "");
        String groupName = Const.RGNAME;
        BSONObject options = new BasicBSONObject("Group", groupName);
        dbcl = db.getCollectionSpace(super.csName).createCollection(CLNAME, options);
        nodeList = getNodeList(db, groupName);
    }

    @AfterClass
    public void teardown() {
        db.getCollectionSpace(super.csName).dropCollection(CLNAME);
        db.disconnect();
    }

    private NodeWarrper getNodeWarrper(String nodename) {
        for (NodeWarrper nodeWarrper : nodeList) {
            if (nodeWarrper.getNodeName().equals(nodename)) {
                return nodeWarrper;
            }
        }
        return null;
    }

    /**
     * 1、coord配置文件中设置preferedInstance值分别为M/S/A
     * 2、重启coord节点
     * 3、连接该coord节点，执行查询操作（如查询多条记录）
     * 4、查看访问节点是否为指定访问实例属性对应节点(执行explain访问计划查看访问节点)
     * <p>
     * 1、设置访问实例为M，coord节点上查看访问连接为该组主节点
     * 2、设置访问实例为S，查看访问连接为该组备节点
     * 3、设置访问实例为A,查看访问连接任意一个节点（可多次操作查看访问节点随机选择）
     */
    @Test(invocationCount = 1)
    public void test14143() {
        String[] expectPreferedInstance = new String[]{"M", "S", "A"};
        for (String s : expectPreferedInstance) {
            BasicBSONObject options = new BasicBSONObject("PreferedInstance", s);
            db.setSessionAttr(options);
            String name = getActualDataNodeName(dbcl);
            if (s.equals("M")) {
                assertTrue(getNodeWarrper(name).isMaster(), name);
            } else if (s.equals("S")) {
                assertFalse(getNodeWarrper(name).isMaster(), name);
            }
            options.append("PreferedInstanceMode", "random").append("Timeout", -1L);
            assertEquals(db.getSessionAttr(),options);
        }
    }
}
