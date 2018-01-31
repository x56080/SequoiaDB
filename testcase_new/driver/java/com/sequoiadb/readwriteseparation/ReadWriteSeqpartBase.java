package com.sequoiadb.readwriteseparation;

import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import org.bson.BasicBSONObject;
import org.testng.SkipException;
import org.testng.annotations.AfterTest;
import org.testng.annotations.BeforeTest;

/**
 * Created by laojingtang on 18-1-29.
 */
public class ReadWriteSeqpartBase extends SdbTestBase {

    @BeforeTest
    public void before() {
        Sequoiadb db = new Sequoiadb(coordUrl, "", "");

        if (CommLib.isStandAlone(db)) {
            throw new SkipException("not support for standalone.");
        }
        ReplicaGroup rg = db.createReplicaGroup("ReadWriteSeqpartRG");
        int[][] param = {{29876, 7}, {39876, 8}, {49876, 9},};
        String hostName=db.getReplicaGroup("SYSCatalogGroup").getMaster().getHostName();

        for (int[] ints : param) {
            BasicBSONObject config = new BasicBSONObject();
            config.append("instanceid", ints[1]);
            rg.createNode(hostName, ints[0], SdbTestBase.workDir + "/" + ints[0], config);
        }

        rg.start();
        db.disconnect();
    }

    @AfterTest
    public void after() {
        Sequoiadb db = new Sequoiadb(coordUrl, "", "");
        db.removeReplicaGroup(Const.RGNAME);
        db.disconnect();
    }
}
