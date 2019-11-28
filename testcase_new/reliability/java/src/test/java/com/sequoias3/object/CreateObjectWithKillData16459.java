package com.sequoias3.object;

import com.amazonaws.AmazonServiceException;
import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.*;
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
import com.sequoias3.commlibs3.s3utils.ObjectUtils;
import com.sequoias3.commlibs3.s3utils.UserUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.Random;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.CopyOnWriteArrayList;

/**
 * test content: 开启版本控制，创建对象过程中db端节点异常 testlink-case: seqDB-16459
 * 
 * @author wangkexin
 * @Date 2019.01.09
 * @version 1.00
 */
public class CreateObjectWithKillData16459 extends S3TestBase {
    private String userName = "user16459";
    private String bucketName = "bucket16459";
    private String keyName = "key16459";
    private String roleName = "normal";
    private List< String > keyNames = new ArrayList<>();
    private Random random = new Random();
    private Map< String, String > keyAndMd5Map = new ConcurrentHashMap< String, String >();
    private List< String > putObjectList = new CopyOnWriteArrayList< String >();
    private int objectNum = 100;
    private String[] accessKeys = null;
    private AmazonS3 s3Client = null;
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {
        CommLibS3.clearUser( userName );
        accessKeys = UserUtils.createUser( userName, roleName );
        s3Client = CommLibS3.buildS3Client( accessKeys[ 0 ], accessKeys[ 1 ] );
        s3Client.createBucket( bucketName );
        CommLibS3.setBucketVersioning( s3Client, bucketName,
                BucketVersioningConfiguration.ENABLED );

        for ( int i = 0; i < objectNum; i++ ) {
            keyNames.add( keyName + "_" + i );
        }
    }

    @Test
    public void testCreateObject() throws Exception {
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
            CreateObjectTask cTask = new CreateObjectTask( keyNames.get( i ) );
            mgr.addTask( cTask );
        }
        mgr.execute();
        Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

        // check whether the cluster is normal and lsn consistency ,the
        // longest waiting time is 600S
        Assert.assertEquals( groupMgr.checkBusinessWithLSN( 600 ), true,
                "checkBusinessWithLSN() occurs timeout" );

        putObjectAndCheck();
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

    private class CreateObjectTask extends OperateTask {
        private String keyName = "";

        public CreateObjectTask( String keyName ) {
            this.keyName = keyName;
        }

        @Override
        public void exec() {
            AmazonS3 s3Client = CommLibS3.buildS3Client( accessKeys[ 0 ],
                    accessKeys[ 1 ] );
            try {
                int writeSize = random.nextInt( 1024 );
                String currContent = ObjectUtils.getRandomString( writeSize );
                String currmd5 = TestTools.getMD5( currContent.getBytes() );
                s3Client.putObject( bucketName, keyName, currContent );
                keyAndMd5Map.put( keyName, currmd5 );
                putObjectList.add( keyName );
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

    private void putObjectAndCheck() throws Exception {
        List< String > remainObjects = new ArrayList< String >();
        remainObjects.addAll( keyNames );
        remainObjects.removeAll( putObjectList );
        for ( String keyName : remainObjects ) {
            int writeSize = random.nextInt( 1024 );
            String currContent = ObjectUtils.getRandomString( writeSize );

            s3Client.putObject( bucketName, keyName, currContent );
            keyAndMd5Map.put( keyName,
                    TestTools.getMD5( currContent.getBytes() ) );
        }

        VersionListing versions = s3Client.listVersions(
                new ListVersionsRequest().withBucketName( bucketName ) );
        List< S3VersionSummary > objects = versions.getVersionSummaries();
        Assert.assertEquals( objects.size(), keyNames.size(),
                "putObjectList : " + putObjectList.toString() + "  ,objects="
                        + printVersionKeys( objects ) );
        for ( S3VersionSummary obj : objects ) {
            String key = obj.getKey();
            String expEtag = keyAndMd5Map.get( key );
            String actEtag = obj.getETag();
            Assert.assertEquals( obj.getVersionId(), "0",
                    "objectName is : " + key );
            Assert.assertEquals( actEtag, expEtag, "objectName is : " + key );
        }
    }

    private String printVersionKeys( List< S3VersionSummary > objects ) {
        String str = "";
        for ( S3VersionSummary obj : objects ) {
            str += obj.getKey();
            str += " ";
        }
        return str;
    }
}