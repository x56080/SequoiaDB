package com.sequoiadb.readwriteseparation;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;
import static com.sequoiadb.readwriteseparation.Helper.getActualDataNodeName;
import static org.testng.Assert.assertNotNull;

/**
 * Created by laojingtang on 18-1-19.
 * Modified by wangkexin on 18-11-27.
 */
public class ReadWriteSeqpart14141 extends SdbTestBase {
    private final java.lang.String CLNAME = this.getClass().getSimpleName();
    private Sequoiadb db;
    private DBCollection dbcl;
    private String rgName = Const.RGNAME + "14141";

    @BeforeClass
    public void setup() {
        db = new Sequoiadb(SdbTestBase.coordUrl, "", "");
        CommLib.createRG(db, rgName);
        BSONObject options = new BasicBSONObject("Group", rgName);
        dbcl = db.getCollectionSpace(SdbTestBase.csName).createCollection(CLNAME, options);
    }

    @AfterClass
    public void teardown() throws InterruptedException {
        db.getCollectionSpace(SdbTestBase.csName).dropCollection(CLNAME);
        db.removeReplicaGroup(rgName);
        db.disconnect();
    }

    /**
     * 1、连接coord执行db.setSessionAttr()，设置PreferedInstance会话实例为instanceid，其中instanceid包含8/9/10，如设置为【8,9,11】或【9/10/11】
     * 2、连接该coord节点，执行查询操作
     * 3、查看访问节点情况
     * <p>
     * 1、查看访问连接节点为指定instanceid对应的数据节点（随机在指定节点中选取）
     */
    @Test
    public void test14141() {
        BSONObject options = new BasicBSONObject("PreferedInstance", new int[]{8, 9, 11});
        db.setSessionAttr(options);
        String actualname = getActualDataNodeName(dbcl);
        assertNotNull(actualname);
    }
}
