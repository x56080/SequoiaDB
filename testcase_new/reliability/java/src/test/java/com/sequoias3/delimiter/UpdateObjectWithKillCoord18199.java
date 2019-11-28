package com.sequoias3.delimiter;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.BucketVersioningConfiguration;
import com.amazonaws.services.s3.model.ListVersionsRequest;
import com.amazonaws.services.s3.model.VersionListing;
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
import com.sequoias3.commlibs3.s3utils.DelimiterUtils;
import com.sequoias3.commlibs3.s3utils.ObjectUtils;
import org.springframework.util.LinkedMultiValueMap;
import org.springframework.util.MultiValueMap;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Random;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * @Description seqDB-18199 :: 更新对象过程中db端节点异常
 * @author fanyu
 * @version 1.00
 * @Date 2019.05.23
 */
public class UpdateObjectWithKillCoord18199 extends S3TestBase {
    private boolean runSuccess = false;
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * new Random().nextInt( 1025 );
    private String filePath = null;
    private String bucketName = "bucket18204";
    private String objectName = "PutObject18199?";
    private String delimiter = "?";
    private int versionNum = 1000;
    private AtomicInteger count = new AtomicInteger( 0 );
    private File localPath = null;
    private GroupMgr groupMgr = null;
    private GroupWrapper coordGroup = null;

    @BeforeClass
    private void setUp() throws Exception {
        localPath = new File( S3TestBase.workDir + File.separator
                + TestTools.getClassName() );
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        filePath = localPath + File.separator + "localFile_" + fileSize
                + ".txt";
        TestTools.LocalFile.createFile( filePath );
        groupMgr = GroupMgr.getInstance();
        coordGroup = groupMgr.getGroupByName( "SYSCoord" );

        s3Client = CommLibS3.buildS3Client();
        CommLibS3.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
        CommLibS3.setBucketVersioning( s3Client, bucketName,
                BucketVersioningConfiguration.ENABLED );
        DelimiterUtils.putBucketDelimiter( bucketName, delimiter );
        DelimiterUtils.checkCurrentDelimiteInfo( bucketName, delimiter );
    }

    @Test
    public void test() throws Exception {
        // kill coord
        TaskMgr mgr = new TaskMgr();
        for ( NodeWrapper node : coordGroup.getNodes() ) {
            FaultMakeTask faultTask = KillNode.getFaultMakeTask( node, 2 );
            mgr.addTask( faultTask );
        }
        for ( int i = 0; i < versionNum; i++ ) {
            mgr.addTask( new PutObject( objectName ) );
        }
        mgr.execute();
        Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );
        for ( int i = count.get(); i < versionNum; i++ ) {
            s3Client.putObject( bucketName, this.objectName,
                    new File( filePath ) );
        }
        // check result
        listVersionsAndCheck();
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( runSuccess ) {
                CommLibS3.clearBucket( s3Client, bucketName );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }

    public class PutObject extends OperateTask {
        private String objectName = null;

        public PutObject( String objectName ) {
            this.objectName = objectName;
        }

        @Override
        public void exec() throws Exception {
            try {
                s3Client.putObject( bucketName, this.objectName,
                        new File( filePath ) );
                count.incrementAndGet();
            } catch ( AmazonS3Exception e ) {
                if ( e.getStatusCode() != 500 ) {
                    throw e;
                }
            }
        }
    }

    private void listVersionsAndCheck() throws IOException {
        String keyMarker = objectName;
        String versionIdMarker = String.valueOf( versionNum );
        VersionListing vsList;
        int i = 0;
        do {
            // list by prefix/keyMarker/versionIdMarker
            vsList = s3Client.listVersions( new ListVersionsRequest()
                    .withBucketName( bucketName ).withDelimiter( delimiter )
                    .withKeyMarker( keyMarker )
                    .withVersionIdMarker( versionIdMarker ) );
            // check
            MultiValueMap< String, String > expMap = new LinkedMultiValueMap< String, String >();
            for ( int j = versionNum - i * 1000 - 1; j >= vsList.getMaxKeys()
                    - i * 1000; j-- ) {
                expMap.add( objectName, String.valueOf( j ) );
            }
            ObjectUtils.checkListVSResults( vsList, new ArrayList< String >(),
                    expMap );
            i++;
            // next keyMark and versionIdMrker
            keyMarker = vsList.getNextKeyMarker();
            versionIdMarker = vsList.getNextVersionIdMarker();
        } while ( vsList.isTruncated() );
    }
}
