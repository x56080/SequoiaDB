package com.sequoias3.object;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.CopyOnWriteArrayList;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.amazonaws.AmazonServiceException;
import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.ListObjectsV2Result;
import com.amazonaws.services.s3.model.S3ObjectSummary;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.fault.KillNode;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;
import com.sequoias3.commlibs3.CommLibS3;
import com.sequoias3.commlibs3.S3TestBase;
import com.sequoias3.commlibs3.TestTools;
import com.sequoias3.commlibs3.s3utils.UserUtils;

/**
 * test content: 更新对象过程中db端节点异常 testlink-case: seqDB-16462
 * 
 * @author wangkexin
 * @Date 2019.01.09
 * @version 1.00
 */
public class UpdateObjectWithKillData16462 extends S3TestBase {
    private String userName = "user16462";
    private String bucketName = "bucket16462";
    private String keyName = "key16462";
    private String roleName = "normal";
    private List< String > keyNames = new ArrayList<>();
    private List< String > updatedObjectList = new CopyOnWriteArrayList< String >();
    private int objectNum = 10;
    private String[] accessKeys = null;
    private AmazonS3 s3Client = null;
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {
        CommLibS3.clearUser( userName );
        accessKeys = UserUtils.createUser( userName, roleName );
        s3Client = CommLibS3.buildS3Client( accessKeys[ 0 ], accessKeys[ 1 ] );
        s3Client.createBucket( bucketName );

        for ( int i = 0; i < objectNum; i++ ) {
            String currentKey = keyName + "_" + i;
            s3Client.putObject( bucketName, currentKey, currentKey + "old" );
            keyNames.add( currentKey );
        }
    }

    @Test
    public void testUpdateObject() throws Exception {
        TaskMgr mgr = new TaskMgr();
        GroupMgr groupMgr = GroupMgr.getInstance();
        List< GroupWrapper > dataGroups = groupMgr.getAllDataGroup();

        for ( int i = 0; i < dataGroups.size(); i++ ) {
            String groupName = dataGroups.get( i ).getGroupName();
            GroupWrapper group = groupMgr.getGroupByName( groupName );
            NodeWrapper node = group.getMaster();
            FaultMakeTask faultTask = KillNode.getFaultMakeTask( node, 1 );
            mgr.addTask( faultTask );
        }

        for ( int i = 0; i < keyNames.size(); i++ ) {
            UpdateObjectTask cTask = new UpdateObjectTask( keyNames.get( i ) );
            mgr.addTask( cTask );
        }
        mgr.execute();
        Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

        // check whether the cluster is normal and lsn consistency ,the
        // longest waiting time is 600S
        Assert.assertEquals( groupMgr.checkBusinessWithLSN( 600 ), true,
                "checkBusinessWithLSN() occurs timeout" );

        updateObjectAgainAndCheck();
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( runSuccess ) {
                UserUtils.deleteUser( userName );
            }
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }

    private class UpdateObjectTask extends OperateTask {
        private String keyName = "";

        public UpdateObjectTask( String keyName ) {
            this.keyName = keyName;
        }

        @Override
        public void exec() {
            AmazonS3 s3Client = CommLibS3.buildS3Client( accessKeys[ 0 ],
                    accessKeys[ 1 ] );
            try {
                String currContent = keyName + "new";
                s3Client.putObject( bucketName, keyName, currContent );
                updatedObjectList.add( keyName );
            } catch ( AmazonServiceException e ) {
                if ( e.getStatusCode() != 500 ) {
                    throw e;
                }
            } finally {
                if ( s3Client != null ) {
                    s3Client.shutdown();
                }
            }
        }
    }

    private void updateObjectAgainAndCheck() throws Exception {
        List< String > remainOldObjects = new ArrayList< String >();
        remainOldObjects.addAll( keyNames );
        remainOldObjects.removeAll( updatedObjectList );
        for ( String keyName : remainOldObjects ) {
            String currContent = keyName + "new";
            s3Client.putObject( bucketName, keyName, currContent );
        }

        ListObjectsV2Result objectsList = s3Client.listObjectsV2( bucketName );
        List< S3ObjectSummary > objects = objectsList.getObjectSummaries();
        Assert.assertEquals( objects.size(), keyNames.size(),
                "updatedObjectList : " + updatedObjectList.toString()
                        + "  ,objects=" + printContentKeys( objects ) );
        for ( int i = 0; i < objects.size(); i++ ) {
            String key = objects.get( i ).getKey();
            String expContent = key + "new";
            String expEtag = TestTools.getMD5( expContent.getBytes() );
            String actEtag = objects.get( i ).getETag();
            Assert.assertEquals( actEtag, expEtag, "objectName is : " + key );
        }
    }

    private String printContentKeys( List< S3ObjectSummary > objects ) {
        String str = "";
        for ( S3ObjectSummary obj : objects ) {
            str += obj.getKey();
            str += " ";
        }
        return str;
    }
}