package com.sequoias3.object;

import java.io.File;
import java.io.IOException;
import java.util.Date;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.CopyObjectRequest;
import com.amazonaws.services.s3.model.GetObjectMetadataRequest;
import com.amazonaws.services.s3.model.ObjectMetadata;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;

/**
 * @Description seqDB-19335:指定ifNoneMatch和ifUnModifiedSince条件复制对象，
 *              源对象不匹配ifNoneMatch
 * @Author huangxiaoni
 * @Date 2019.09.17
 */
public class CopyObject19335 extends S3TestBase {
    private boolean runSuccess = false;
    private AmazonS3 s3Client = null;
    private String srcBucketName = "bucket19335a";
    private String dstBucketName = "bucket19335b";
    private String keyName = "obj19335";
    private int fileSize = 5 * 1024 * 1024;
    private File localPath = null;
    private String filePath1 = null;
    private String filePath2 = null;

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator + TestTools
                .getClassName() );
        filePath1 =
                localPath + File.separator + "localFile_" + fileSize + "_1.txt";
        filePath2 =
                localPath + File.separator + "localFile_" + fileSize + "_2.txt";
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath1, fileSize );
        TestTools.LocalFile.createFile( filePath2, fileSize );

        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, srcBucketName );
        CommLib.clearBucket( s3Client, dstBucketName );
        s3Client.createBucket( srcBucketName );
        s3Client.createBucket( dstBucketName );
        CommLib.setBucketVersioning( s3Client, srcBucketName, "Enabled" );
        CommLib.setBucketVersioning( s3Client, dstBucketName, "Enabled" );

        s3Client.putObject( srcBucketName, keyName, new File( filePath1 ) );
        s3Client.putObject( srcBucketName, keyName, new File( filePath2 ) );
    }

    @Test
    private void test() throws Exception {
        // get last modified date of the current version object
        GetObjectMetadataRequest metadataRequest = new GetObjectMetadataRequest(
                srcBucketName, keyName );
        ObjectMetadata objMetadata = s3Client
                .getObjectMetadata( metadataRequest );
        Date srcObjCurLastModDate = objMetadata.getLastModified();
        String srcObjCurVerETag = objMetadata.getETag();

        // copy object
        CopyObjectRequest request = new CopyObjectRequest( srcBucketName,
                keyName, dstBucketName, keyName );
        request.withUnmodifiedSinceConstraint( srcObjCurLastModDate );
        request.withNonmatchingETagConstraint( srcObjCurVerETag );
        try {
            s3Client.copyObject( request );
            Assert.fail( "expect fail, but actual success." );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "304 Not Modified" );
        }

        // check results
        try {
            s3Client.getObject( dstBucketName, keyName );
            Assert.fail( "expect fail, but actual success." );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "NoSuchKey" );
        }
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                CommLib.clearBucket( s3Client, srcBucketName );
                CommLib.clearBucket( s3Client, dstBucketName );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            s3Client.shutdown();
        }
    }
}