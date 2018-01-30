package com.sequoiadb.readwriteseparation;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.List;
import java.util.Random;

import static com.sequoiadb.readwriteseparation.Helper.getNodeList;

/**
 * Created by laojingtang on 18-1-19.
 */
public class ReadWriteSeqpart14148 extends SdbTestBase {
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
     * 1、 coord连接上使用db.setSessionAttr()进行timeout设置
     * 2、  连接该coord节点，执行lob操作（分别插入、删除、读取lob），操作耗时较长，超过timeout值
     * 3、查看操作结果
     * 1、到达指定timeout时间后，操作失败，连接断开
     */
    @Test(invocationCount = 1)
    public void test14148() throws InterruptedException {
        BSONObject options = new BasicBSONObject("Timeout", 200);
        db.setSessionAttr(options);

        try {
            DBLob lob = dbcl.createLob();
            lob.write(new byte[1024 * 1024 * 10]);
            lob.close();
        } catch (BaseException e) {
            if (e.getErrorCode() != -13) {
                throw e;
            } else {
                e.printStackTrace();
            }
        } finally {
            db.setSessionAttr(new BasicBSONObject("Timeout", 10000));
        }
    }
}
