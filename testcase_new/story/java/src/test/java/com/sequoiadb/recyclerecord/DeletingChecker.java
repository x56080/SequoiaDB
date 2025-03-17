package com.sequoiadb.recyclerecord;

import java.util.*;

import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.Ssh;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @description seqDB-000:
 * @author wuyan
 * @date 2021.4.12
 * @version 1.10
 */

public class DeletingChecker {
    private Ssh ssh = null;
    private String dumpCmd = "";
    private String outputFile = null;

    DeletingChecker() {}

    public void init( String coordUrl, String csName, String clName, String rootPwd ) throws Exception {
        Sequoiadb db = new Sequoiadb( coordUrl, "", "" );
        // parse cl data group
        BSONObject empty = new BasicBSONObject();
        DBCursor cursor = db.getSnapshot( Sequoiadb.SDB_SNAP_CATALOG,
                new BasicBSONObject( "Name", csName + "." + clName ),
                empty, empty );
        BSONObject obj = cursor.getNext();
        BSONObject groupObj = (BSONObject) ((BSONObject) obj.get( "CataInfo" )).get("0");
        String groupName = ((String) groupObj.get( "GroupName" ));
        ReplicaGroup rg = db.getReplicaGroup( groupName );
        String hostName = rg.getMaster().getHostName();
        BSONObject detail = rg.getDetail();
        // parse dbpath
        String dbPath = null;
        int primaryNID = (int) detail.get( "PrimaryNode" );
        List<BSONObject> groupInfo = ((List<BSONObject>) detail.get( "Group" ));
        for (int i = 0; i < groupInfo.size(); i++) {
            BSONObject node = groupInfo.get( i );
            int nid = (int) node.get( "NodeID" );
            if ( nid == primaryNID ) {
                dbPath = (String) node.get( "dbpath" );
                break;
            }
        }
        // init ssh
        ssh = new Ssh( hostName, "root", rootPwd );
        // parse install dir
        ssh.exec( "cat /etc/default/sequoiadb |grep INSTALL_DIR" );
        String str = ssh.getStdout();
        if ( str.length() <= 0 ) {
            throw new Exception(
                "exec command:cat /etc/default/sequoiadb |grep INSTALL_DIR can not find sequoiadb install dir"
            );
        }
        String installPath = str.substring( str.indexOf( "=" ) + 1,
                str.length() - 1 );
        // build cmd
        outputFile = "/tmp/" + csName + "." + clName + ".dump";
        dumpCmd += installPath + "/bin/sdbdmsdump ";
        dumpCmd += " -d " + dbPath;
        dumpCmd += " -o " + outputFile;
        dumpCmd += " -c " + csName + " -l " + clName;
        dumpCmd += " -t true -p true -a dump";
    }

    public int getTotalDeletingRecord() throws Exception {
        ssh.exec( dumpCmd );
        ssh.exec( "grep -i 'Total deleting record' " + outputFile + ".0 | awk '{print $4}'" );
        String str = ssh.getStdout();
        if ( str.length() <= 0 ) {
            throw new Exception( "Failed to dump cl" );
        }
        String count = str.substring( 0, str.length() - 1 );
        return Integer.parseInt( count );
    }

    public void fini() throws Exception {
        ssh.exec( "rm " + outputFile + ".0" );
    }
}
