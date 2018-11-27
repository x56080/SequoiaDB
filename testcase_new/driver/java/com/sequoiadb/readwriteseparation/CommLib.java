package com.sequoiadb.readwriteseparation;

import org.bson.BasicBSONObject;
import org.testng.SkipException;

import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

public class CommLib extends SdbTestBase {
	
    public static  void createRG(Sequoiadb db, String rgName) {
    	int reservedPortBegin = SdbTestBase.reservedPortBegin;
    	
        if (isStandAlone(db)) {
            throw new SkipException("not support for standalone.");
        }
        ReplicaGroup rg = db.createReplicaGroup(rgName);
        int[][] param = {{reservedPortBegin+10, 7}, {reservedPortBegin+20, 8}, {reservedPortBegin+30, 9},};
        String hostName=db.getReplicaGroup("SYSCatalogGroup").getMaster().getHostName();

        for (int[] ints : param) {
            BasicBSONObject config = new BasicBSONObject();
            config.append("instanceid", ints[1]);
            rg.createNode(hostName, ints[0], SdbTestBase.reservedDir + "/" + ints[0], config);
        }
        rg.start();
    }

    public static boolean isStandAlone(Sequoiadb sdb){
		try{
			sdb.listReplicaGroups();
		}catch(BaseException e){
			if( e.getErrorCode() == -159 ){  //-159:The operation is for coord node only
				return true;
			}
		}
		return false;
	}
}
