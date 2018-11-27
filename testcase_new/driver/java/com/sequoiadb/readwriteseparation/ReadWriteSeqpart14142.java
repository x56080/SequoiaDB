package com.sequoiadb.readwriteseparation;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.List;
import java.util.Random;

import static com.sequoiadb.readwriteseparation.Helper.getActualDataNodeName;
import static com.sequoiadb.readwriteseparation.Helper.getNodeList;
import static org.testng.Assert.assertEquals;

/**
 * Created by laojingtang on 18-1-19.
 * Modified by wangkexin on 18-11-27.
 */
public class ReadWriteSeqpart14142 extends SdbTestBase {
    private final String CLNAME = this.getClass().getSimpleName();
    private Sequoiadb db;
    private DBCollection dbcl;
    private Random random = new Random();
    private List<NodeWarrper> nodeList;
    private String rgName = Const.RGNAME + "14142";

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

    /**
     * 1、连接该coord节点，设置preferedInstance值为数据节点的instanceid值，设置timeout值
     * 2、执行查询操作（如查询多条记录），其中查询操作时间分别验证小于timeout时间、超过timeout时间
     * 3、查看访问节点是否为指定instanceid对应节点(如执行explain访问计划查看连接节点db.wy.wy.find({a:{"$lt":2}}).explain())
     * 5、执行getSessionAtr（）获取session信息
     * <p>
     * 1、查询时间小于timeout值时，查询操作成功，执行explain查看访问连接为指定instanceid对应的节点
     * 2、查询时间超过timeout值时，查询会话失败断开
     * 3、获取session信息和实际设置session信息一致（其中mode取默认值random）
     */
    @Test(invocationCount = 1)
    public void test14142() {
        int expectid = getRandomInstanceid();

        BasicBSONObject options = new BasicBSONObject("PreferedInstance", expectid).append("Timeout", 200L);
        db.setSessionAttr(options);
        try {
            DBLob lob = dbcl.createLob();
            lob.write(new byte[1024 * 1024 * 10]);
            lob.close();
        } catch (BaseException e) {
        	Assert.assertEquals(e.getErrorCode(), -13);
        }

        options.put("Timeout", 20000L);
        db.setSessionAttr(options);
        DBLob lob = dbcl.createLob();
        lob.write(new byte[1024 * 1024 * 10]);
        lob.close();
        db.setSessionAttr(options);
        String actualNodeName = getActualDataNodeName(dbcl);
        assertEquals(getNodeWarrper(actualNodeName).getInstenceid(), expectid);
        BSONObject actual = db.getSessionAttr();
        options.append("PreferedInstanceMode", "random");
        assertEquals(actual, options);
    }
}
