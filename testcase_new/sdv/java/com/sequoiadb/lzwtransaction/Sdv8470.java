package com.sequoiadb.lzwtransaction;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbConfTestBase;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;


public class Sdv8470 extends SdbConfTestBase {
    private Sequoiadb sdb;
    private CollectionSpace cs;
    private final static String CLNAME = Sdv8470.class.getSimpleName();

    @Override
    protected void setNodeConf() {
        dataConf.put("signalinterval", 1);
        stdalnConf.put("signalinterval", 1);
    }

    @BeforeClass
    public void setUp() {
        this.sdb = new Sequoiadb(SdbTestBase.coordUrl, "", "");
        this.cs = this.sdb.getCollectionSpace(SdbTestBase.csName);
        cs.createCollection(CLNAME);
    }

    /**
     * 1、3组3节点的集群，开启参数 --signalinterval 1的配置（所有节点），重启节点使配置生效
     * 2、7个并发同时执行：连接sdb -> 插入数据 -> 关闭连接，如：
     * for(i=0;i<100000;i++){db = new Sdb("localhost:11810");db.hxn.hxn.insert({a:1});db.close();}
     * 3、重试多次执行
     */
    @Test
    public void test() {
        SdbThreadBase task = new SdbThreadBase() {
            @Override
            public void exec() throws Exception {
                for (int i = 0; i < 1000000; i++) {
                    Sequoiadb db = null;
                    try {
                        db = new Sequoiadb(SdbTestBase.coordUrl, "", "");
                        db.getCollectionSpace(SdbTestBase.csName)
                                .getCollection(CLNAME).insert("{a:1}");
                    } catch (BaseException e) {
                        if (e.getErrorCode() != -15)
                            throw e;
                    } finally {
                        if (db != null)
                            db.disconnect();
                    }
                }
            }
        };

        task.start(10);

        Assert.assertTrue(task.isSuccess(), task.getErrorMsg());
    }

    @AfterClass
    public void tearDown() {
        if (sdb != null) {
            this.cs.dropCollection(CLNAME);
            sdb.disconnect();
        }
    }
}
