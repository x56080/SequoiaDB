package com.sequoias3.head;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.GetObjectMetadataRequest;
import com.amazonaws.services.s3.model.ObjectMetadata;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.io.IOException;

/**
 * @Description seqDB-16674: test HeadObject
 * @author wuyan
 * @Date 2018.12.17
 * @version 1.00
 */
public class HeadObject16674 extends S3TestBase {
    private boolean runSuccess = false;
    private String key = "/test/object16674";
    private int fileSize1 = 1024 * 300;
    private int fileSize2 = 1024 * 100;
    private File localPath = null;
    private String filePath1 = null;
    private String filePath2 = null;
    private AmazonS3 s3Client = null;

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
        if ( s3Client.doesObjectExist( S3TestBase.bucketName, key ) ) {
            s3Client.deleteObject( S3TestBase.bucketName, key );
        }

    }

    @Test
    public void testCreateBucket() throws IOException {
        s3Client.putObject( S3TestBase.bucketName, key, new File( filePath1 ) );
        GetObjectMetadataRequest request = new GetObjectMetadataRequest(
                S3TestBase.bucketName, key );
        ObjectMetadata result = s3Client.getObjectMetadata( request );
        checkObjectMetaInfo( result, filePath1, fileSize1 );

        s3Client.deleteObject( S3TestBase.bucketName, key );
        try {
            s3Client.getObjectMetadata( request );
            Assert.fail( "head object must be fail!" );
        } catch ( AmazonS3Exception e ) {
            // 404 Not Found
            Assert.assertEquals( e.getStatusCode(), 404 );
        }

        s3Client.putObject( S3TestBase.bucketName, key, new File( filePath2 ) );
        GetObjectMetadataRequest request2 = new GetObjectMetadataRequest(
                S3TestBase.bucketName, key );
        ObjectMetadata result2 = s3Client.getObjectMetadata( request2 );
        checkObjectMetaInfo( result2, filePath2, fileSize2 );

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                s3Client.deleteObject( S3TestBase.bucketName, key );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            s3Client.shutdown();
        }
    }

    private void checkObjectMetaInfo( ObjectMetadata result, String filePath,
            int fileSize ) throws IOException {
        long size = result.getContentLength();
        String md5 = result.getETag();

        String expMd5 = TestTools.getMD5( filePath );
        Assert.assertEquals( size, fileSize );
        Assert.assertEquals( md5, expMd5 );
    }

}
