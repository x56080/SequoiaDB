package com.sequoias3.object.concurrent;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.ListVersionsRequest;
import com.amazonaws.services.s3.model.VersionListing;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.S3ThreadBase;
import com.sequoias3.testcommon.TestTools;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

/**
 * @Description seqDB-16495:concurrent delete and list object versions
 * @author wuyan
 * @Date 2019.1.8
 * @version 1.00
 */
public class DeleteAndListVersions16495 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket16495";
    private String keyName = "aa%bb%object16495";
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * 1024;
    private File localPath = null;
    private String filePath = null;
    private int objectNums = 10;

    @BeforeClass
    private void setUp() throws Exception {
        localPath = new File( S3TestBase.workDir + File.separator
                + TestTools.getClassName() );
        filePath = localPath + File.separator + "localFile_" + fileSize
                + ".txt";
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath, fileSize );

        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
    }

    @Test
    public void testCreateBucket() throws Exception {
        List< DeleteObjectThread > deleteObjectThreads = new ArrayList<>(
                objectNums );
        ListObjectThread listObjectThread = new ListObjectThread();
        for ( int i = 0; i < objectNums; i++ ) {
            String key = keyName + "_" + i;
            s3Client.putObject( bucketName, key, new File( filePath ) );
            deleteObjectThreads.add( new DeleteObjectThread( key ) );
        }
        for ( DeleteObjectThread deleteObjectThread : deleteObjectThreads ) {
            deleteObjectThread.start();
        }
        listObjectThread.start();

        for ( DeleteObjectThread deleteObjectThread : deleteObjectThreads ) {
            Assert.assertTrue( deleteObjectThread.isSuccess(),
                    deleteObjectThread.getErrorMsg() );
        }
        Assert.assertTrue( listObjectThread.isSuccess(),
                listObjectThread.getErrorMsg() );

        listObjectsAndCheckResult();
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( runSuccess ) {
                CommLib.clearBucket( s3Client, bucketName );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }

    private void listObjectsAndCheckResult() throws IOException {
        VersionListing versionList = s3Client.listVersions(
                new ListVersionsRequest().withBucketName( bucketName ) );
        // object has been deleted.the nums is 0
        Assert.assertEquals( versionList.getVersionSummaries().size(), 0 );
        Assert.assertFalse( versionList.isTruncated(),
                "list is empty and has no objects!" );
    }

    private class DeleteObjectThread extends S3ThreadBase {
        private String keyName;

        public DeleteObjectThread( String keyName ) {
            this.keyName = keyName;
        }

        @Override
        public void exec() throws Exception {
            AmazonS3 s3Client = CommLib.buildS3Client();
            try {
                s3Client.deleteObject( bucketName, keyName );
            } finally {
                if ( s3Client != null ) {
                    s3Client.shutdown();
                }
            }
        }
    }

    private class ListObjectThread extends S3ThreadBase {
        @Override
        public void exec() throws Exception {
            AmazonS3 s3Client = CommLib.buildS3Client();
            try {
                s3Client.listVersions( new ListVersionsRequest()
                        .withBucketName( bucketName ) );
            } finally {
                if ( s3Client != null ) {
                    s3Client.shutdown();
                }
            }
        }
    }
}
