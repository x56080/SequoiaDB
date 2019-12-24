package com.sequoias3.object.concurrent;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.BucketVersioningConfiguration;
import com.amazonaws.services.s3.model.ListVersionsRequest;
import com.amazonaws.services.s3.model.S3VersionSummary;
import com.amazonaws.services.s3.model.VersionListing;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.S3ThreadBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Iterator;
import java.util.List;

/**
 * @Description seqDB-16509:enabling bucket versioning,concurrent delete and
 *              list object versions
 * @author wuyan
 * @Date 2019.1.10
 * @version 1.00
 */
public class DeleteAndListVersions16509 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket16509";
    private String keyName = "aa%bb%object16509";
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * 1024;
    private File localPath = null;
    private String filePath = null;
    private int objectNums = 5;

    @BeforeClass
    private void setUp() throws Exception {
        localPath = new File( S3TestBase.workDir + File.separator + TestTools
                .getClassName() );
        filePath =
                localPath + File.separator + "localFile_" + fileSize + ".txt";
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath, fileSize );

        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
        CommLib.setBucketVersioning( s3Client, bucketName,
                BucketVersioningConfiguration.ENABLED );
    }

    @Test
    public void testCreateBucket() throws Exception {
        List<DeleteObjectThread> deleteObjectThreads = new ArrayList<>(
                objectNums );
        ListObjectThread listObjectThread = new ListObjectThread();

        List<String> expKeys = new ArrayList<>();
        for ( int i = 0; i < objectNums; i++ ) {
            String key = keyName + "_" + i;
            s3Client.putObject( bucketName, key, new File( filePath ) );
            expKeys.add( key );
            expKeys.add( key );
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

        listObjectsAndCheckResult( expKeys );
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

    private void listObjectsAndCheckResult( List<String> expKeys )
            throws Exception {
        List<String> actVersionKeys = new ArrayList<>();
        String deleteTagVersion = "1";
        VersionListing versionList = s3Client.listVersions(
                new ListVersionsRequest().withBucketName( bucketName ) );
        while ( true ) {
            Iterator<S3VersionSummary> versionIter = versionList
                    .getVersionSummaries().iterator();

            while ( versionIter.hasNext() ) {
                S3VersionSummary vs = versionIter.next();
                String actKey = vs.getKey();
                String versionId = vs.getVersionId();
                if ( versionId.equals( deleteTagVersion ) ) {
                    // check delete tag
                    checkDeleteTag( bucketName, keyName );
                    Assert.assertTrue( vs.isDeleteMarker(),
                            "the key must be deleteTag!" );
                } else {
                    // check the history version object
                    checkDeleteObjectResult( bucketName, actKey, versionId );
                    Assert.assertFalse( vs.isDeleteMarker() );
                }
                actVersionKeys.add( actKey );
            }
            if ( versionList.isTruncated() ) {
                versionList = s3Client.listNextBatchOfVersions( versionList );
            } else {
                break;
            }
            Collections.sort( actVersionKeys );
            Collections.sort( expKeys );
            Assert.assertEquals( actVersionKeys, expKeys );
        }
    }

    private void checkDeleteObjectResult( String bucketName, String key,
            String versionId ) throws Exception {
        // current version object has been deleted
        boolean isExistObject = s3Client.doesObjectExist( bucketName, key );
        Assert.assertFalse( isExistObject, "the object should not exist!" );

        // deleted object has been a history version object,the versionId is "0"
        String downfileMd5 = ObjectUtils
                .getMd5OfObject( s3Client, localPath, bucketName, key,
                        versionId );
        Assert.assertEquals( downfileMd5, TestTools.getMD5( filePath ) );
    }

    private void checkDeleteTag( String bucketName, String key ) {
        try {
            s3Client.getObject( bucketName, key );
            Assert.fail( "get delete tag object must be fail!" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "NoSuchKey" );
        }
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
