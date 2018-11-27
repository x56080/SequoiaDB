package com.sequoiadb.readwriteseparation;

import com.sequoiadb.base.DBCollection;
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
 * Modified by wangkexin on 18-11-27.
 */
public class ReadWriteSeqpart14147 extends SdbTestBase {
    private final String CLNAME = this.getClass().getSimpleName();
    private Sequoiadb db;
    private DBCollection dbcl;
    private Random random = new Random();
    private List<NodeWarrper> nodeList;
    private String rgName = Const.RGNAME + "14147";

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
     * 1、 coord连接上使用db.setSessionAttr()进行timeout设置
     * 2、  连接该coord节点，分别执行如下操作：
     * 1）创建、删除cs、cl；挂载cl、修改cl
     * 2）执行插入、更新、删除操作
     * 3）执行切分操作
     * 操作耗时较长，超过timeout值
     * 3、查看操作结果
     * 1、到达指定timeout时间后，操作失败，会话连接断开
     */
    @Test
    public void test14147() {
        String cs = "14147CS", cl = "14147cl";
        db.setSessionAttr(new BasicBSONObject("Timeout", 200L));
        try{
            db.createCollectionSpace(cs).createCollection(cl);
        }catch (BaseException e){
            if(e.getErrorCode()!=-13)
                throw e;
        }finally {
            db.setSessionAttr(new BasicBSONObject("Timeout",-1));
            try{
                db.dropCollectionSpace(cs);
            }catch (BaseException e){}
        }
    }

}
