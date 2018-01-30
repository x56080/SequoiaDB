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

import static com.sequoiadb.readwriteseparation.Helper.getActualDataNodeName;
import static com.sequoiadb.readwriteseparation.Helper.getNodeList;
import static org.testng.Assert.assertNotNull;

/**
 * Created by laojingtang on 18-1-19.
 */
public class ReadWriteSeqpart14141 extends SdbTestBase {
    private final java.lang.String CLNAME = this.getClass().getSimpleName();
    private Sequoiadb db;
    private DBCollection dbcl;
    private List<NodeWarrper> nodeList;

    @BeforeClass
    public void setup() {
        db = new Sequoiadb(super.coordUrl, "", "");
        BSONObject options = new BasicBSONObject("Group", Const.RGNAME);
        dbcl = db.getCollectionSpace(super.csName).createCollection(CLNAME, options);
        nodeList = getNodeList(db, Const.RGNAME);
    }

    @AfterClass
    public void teardown() {
        db.getCollectionSpace(super.csName).dropCollection(CLNAME);
        db.disconnect();
    }

    /**
     * 1、连接coord执行db.setSessionAttr()，设置PreferedInstance会话实例为instanceid，其中instanceid包含8/9/10，如设置为【8,9,11】或【9/10/11】
     * 2、连接该coord节点，执行查询操作
     * 3、查看访问节点情况
     * <p>
     * 1、查看访问连接节点为指定instanceid对应的数据节点（随机在指定节点中选取）
     */
    @Test(invocationCount = 100)
    public void test14141() {
        BSONObject options = new BasicBSONObject("PreferedInstance", new int[]{8, 9, 11});
        db.setSessionAttr(options);
        String actualname = getActualDataNodeName(dbcl);
        assertNotNull(actualname);
    }
}
