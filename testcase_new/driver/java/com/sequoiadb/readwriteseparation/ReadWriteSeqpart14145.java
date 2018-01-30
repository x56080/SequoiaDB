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
import static org.testng.Assert.fail;

/**
 * Created by laojingtang on 18-1-19.
 */
public class ReadWriteSeqpart14145 extends SdbTestBase {
    private final String CLNAME = this.getClass().getSimpleName();
    private Sequoiadb db;
    private DBCollection dbcl;
    private List<NodeWarrper> nodeList;

    @BeforeClass
    public void setup() {
        db = new Sequoiadb(super.coordUrl, "", "");
        String groupName=Const.RGNAME;
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
     * 1、coord配置文件中设置preferedInstance值为多个instanceid，如节点b和c的instanceid；设置节点选择模式为random模式
     * 2、重启coord节点
     * 3、连接该coord节点，执行查询操作（如查询多条记录）
     * 4、查看访问节点是否为指定instanceid对应节点（执行explain查看）
     */
    @Test(invocationCount = 1)
    public void test14145() {
        int[] id = new int[]{nodeList.get(0).getInstenceid(), nodeList.get(1).getInstenceid()};

        BSONObject options = new BasicBSONObject("PreferedInstance", id).append("PreferedInstanceMode", "random");
        db.setSessionAttr(options);
        String actualNodeName = getActualDataNodeName(dbcl);
        int actualId = getNodeWarrper(actualNodeName).getInstenceid();
        if (actualId != id[0] && actualId != id[1]) {
            fail("actual:" + actualId + " expect: " + id[0] + " or " + id[1]);
        }
        System.out.println(db.getSessionAttr());

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
