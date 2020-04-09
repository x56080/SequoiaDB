package com.sequoias3.object.concurrent;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CopyObjectRequest;
import com.amazonaws.services.s3.model.ListVersionsRequest;
import com.amazonaws.services.s3.model.S3VersionSummary;
import com.amazonaws.services.s3.model.VersionListing;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/**
 * @Description seqDB-19350:目标桶开启版本控制，并发增加和复制相同目标对象
 * @author wuyan
 * @Date 2019.09.20
 * @version 1.00
 */
public class CopyObject19350 extends S3TestBase {

    private boolean runSuccess = false;
    private String srcKeyName = "/SRC/bb%object19350";
    private String destKeyName = "/dest/bb/object19350";
    private String bucketName = "bucket19350";
    private AmazonS3 s3Client = null;
    private int fileSize1 = 1024 * 1024 * 13;
    private int fileSize2 = 1024 * 1024 * 20;
    private File localPath = null;
    private String filePath1 = null;
    private String filePath2 = null;

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator
                + TestTools.getClassName() );
        filePath1 = localPath + File.separator + "localFile_" + fileSize1
                + ".txt";
        filePath2 = localPath + File.separator + "localFile_" + fileSize2
                + ".txt";
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath1, fileSize1 );
        TestTools.LocalFile.createFile( filePath2, fileSize2 );

        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );
        s3Client.putObject( bucketName, srcKeyName, new File( filePath1 ) );
    }

    @Test
    public void testCopyObject() throws Exception {

        ThreadExecutor threadExec = new ThreadExecutor();
        threadExec.addWorker( new CopyObject( destKeyName ) );
        threadExec.addWorker( new PutObject( destKeyName ) );
        threadExec.run();

        checkResult();
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                CommLib.clearBucket( s3Client, bucketName );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            s3Client.shutdown();
        }
    }

    private void checkResult() throws Exception {
        String currentVersionId = "1";
        String hisVersionId = "0";
        List< String > expVersionIds = new ArrayList<>();
        expVersionIds.add( hisVersionId );
        expVersionIds.add( currentVersionId );

        List< String > actVersionIds = new ArrayList<>();
        // list match destKey by prefix
        VersionListing versionList = s3Client
                .listVersions( new ListVersionsRequest()
                        .withBucketName( bucketName ).withPrefix( "/dest" ) );
        List< S3VersionSummary > verList = versionList.getVersionSummaries();
        for ( S3VersionSummary versionSummary : verList ) {
            String versionId = versionSummary.getVersionId();
            long size = versionSummary.getSize();
            if ( versionId.equals( currentVersionId ) ) {
                if ( size == fileSize1 ) {
                    Assert.assertEquals( versionSummary.getETag(),
                            TestTools.getMD5( filePath1 ) );
                    checkObjectContent( currentVersionId, filePath1 );
                } else {
                    Assert.assertEquals( versionSummary.getETag(),
                            TestTools.getMD5( filePath2 ) );
                    Assert.assertEquals( versionSummary.getSize(), fileSize2 );
                    checkObjectContent( currentVersionId, filePath2 );
                }
            } else {
                // the object of history version
                if ( size == fileSize1 ) {
                    Assert.assertEquals( versionSummary.getETag(),
                            TestTools.getMD5( filePath1 ) );
                    checkObjectContent( hisVersionId, filePath1 );
                } else {
                    Assert.assertEquals( versionSummary.getETag(),
                            TestTools.getMD5( filePath2 ) );
                    Assert.assertEquals( versionSummary.getSize(), fileSize2 );
                    checkObjectContent( hisVersionId, filePath2 );
                }
            }
            actVersionIds.add( versionId );
        }

        Collections.sort( expVersionIds );
        Collections.sort( actVersionIds );
        // check only two version of the destKey
        Assert.assertEquals( actVersionIds, expVersionIds );
    }

    private void checkObjectContent( String versionId, String filePath )
            throws Exception {
        String downfileMd5 = ObjectUtils.getMd5OfObject( s3Client, localPath,
                bucketName, destKeyName, versionId );
        Assert.assertEquals( downfileMd5, TestTools.getMD5( filePath ) );
    }

    private class CopyObject {
        private AmazonS3 s3Client1 = CommLib.buildS3Client();
        private String destKeyName;

        private CopyObject( String destKeyName ) {
            this.destKeyName = destKeyName;

        }

        @ExecuteOrder(step = 1)
        private void copyObject() throws Exception {
            try {
                CopyObjectRequest request = new CopyObjectRequest( bucketName,
                        srcKeyName, bucketName, destKeyName );
                s3Client1.copyObject( request );
            } finally {
                if ( s3Client1 != null ) {
                    s3Client1.shutdown();
                }
            }
        }
    }

    private class PutObject {
        private AmazonS3 s3Client1 = CommLib.buildS3Client();
        private String keyName;

        private PutObject( String keyName ) {
            this.keyName = keyName;

        }

        @ExecuteOrder(step = 1)
        private void putObject() throws Exception {
            try {
                s3Client1.putObject( bucketName, keyName,
                        new File( filePath2 ) );
            } finally {
                if ( s3Client1 != null ) {
                    s3Client1.shutdown();
                }
            }
        }
    }
}
