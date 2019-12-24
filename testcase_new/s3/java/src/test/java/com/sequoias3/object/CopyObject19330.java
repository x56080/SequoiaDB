package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CopyObjectRequest;
import com.amazonaws.services.s3.model.CopyObjectResult;
import com.amazonaws.services.s3.model.GetObjectMetadataRequest;
import com.amazonaws.services.s3.model.ObjectMetadata;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.AfterMethod;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.io.IOException;
import java.util.Date;

/**
 * @Description seqDB-19330:复制对象指定ifUnModifiedSince条件
 * @Author huangxiaoni
 * @Date 2019.09.17
 */
public class CopyObject19330 extends S3TestBase {
    private int runSuccessNum = 0;
    private int expRunSuccessNum = 3;
    private AmazonS3 s3Client = null;
    private String srcBucketName = "bucket19330a";
    private String dstBucketName = "bucket19330b";
    private String srcKeyName = "srcObj19330";
    private String dstKeyName = "dstObj19330";
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

        s3Client.putObject( srcBucketName, srcKeyName, new File( filePath1 ) );
        s3Client.putObject( srcBucketName, srcKeyName, new File( filePath2 ) );
    }

    // init keyNameB after copy
    @AfterMethod
    private void afterMethod() {
        String dstObjHisVer = "0";
        s3Client.deleteVersion( dstBucketName, dstKeyName, dstObjHisVer );
    }

    // a.versionId is history version, but not modified of history version after
    // appoint the date
    @Test
    private void testCopyObject_A() throws Exception {
        // get last modified date of the current version object
        Date srcCurLastModDate = getObjLastModDate( srcBucketName, srcKeyName );

        // copy object
        String srcObjHisVer = "0";
        CopyObjectRequest objRequest = new CopyObjectRequest( srcBucketName,
                srcKeyName, srcObjHisVer, dstBucketName, dstKeyName );
        objRequest.withUnmodifiedSinceConstraint( srcCurLastModDate );
        s3Client.copyObject( objRequest );

        // check results
        checkObjectAttribute( dstBucketName, dstKeyName, filePath1 );
        checkObjectContent( dstBucketName, dstKeyName, filePath1 );
        runSuccessNum++;
    }

    // b.modified after appoint the date
    @Test
    private void testCopyObject_B() throws Exception {
        // get last modified date of the current version object
        Date srcCurLastModDate = getObjLastModDate( srcBucketName, srcKeyName );

        // put object after last modified date of current version object
        s3Client.putObject( srcBucketName, srcKeyName, new File( filePath2 ) );

        // copy object
        CopyObjectRequest objRequest = new CopyObjectRequest( srcBucketName,
                srcKeyName, dstBucketName, dstKeyName );
        objRequest.withUnmodifiedSinceConstraint(
                new Date( srcCurLastModDate.getTime() - 1000 ) );
        CopyObjectResult result = s3Client.copyObject( objRequest );
        // check results
        Assert.assertEquals( result, null );
        Assert.assertFalse(
                s3Client.doesObjectExist( dstBucketName, dstKeyName ) );
        runSuccessNum++;
    }

    // c.not modified after appoint the date
    @Test
    private void testCopyObject_C() throws Exception {
        // get last modified date of the current version object
        Date srcCurLastModDate = getObjLastModDate( srcBucketName, srcKeyName );

        // copy object
        CopyObjectRequest objRequest = new CopyObjectRequest( srcBucketName,
                srcKeyName, dstBucketName, dstKeyName );
        objRequest.withUnmodifiedSinceConstraint( srcCurLastModDate );
        s3Client.copyObject( objRequest );

        // check results
        checkObjectAttribute( dstBucketName, dstKeyName, filePath2 );
        checkObjectContent( dstBucketName, dstKeyName, filePath2 );
        runSuccessNum++;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccessNum == expRunSuccessNum ) {
                CommLib.clearBucket( s3Client, srcBucketName );
                CommLib.clearBucket( s3Client, dstBucketName );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            s3Client.shutdown();
        }
    }

    private void checkObjectContent( String bucketName, String keyName,
            String filePath ) throws Exception {
        String downfileMd5 = ObjectUtils
                .getMd5OfObject( s3Client, localPath, bucketName, keyName );
        Assert.assertEquals( downfileMd5, TestTools.getMD5( filePath ) );
    }

    private void checkObjectAttribute( String bucketName, String keyName,
            String filePath ) throws IOException {
        GetObjectMetadataRequest request = new GetObjectMetadataRequest(
                bucketName, keyName );
        ObjectMetadata objMetadata = s3Client.getObjectMetadata( request );
        String expMd5 = TestTools.getMD5( filePath );
        Assert.assertEquals( objMetadata.getETag(), expMd5 );
        Assert.assertEquals( objMetadata.getContentLength(), fileSize );
        Assert.assertEquals( objMetadata.getVersionId(), "0",
                "the keyName=" + keyName );
    }

    private Date getObjLastModDate( String bucketName, String keyName ) {
        GetObjectMetadataRequest metadataRequest = new GetObjectMetadataRequest(
                bucketName, srcKeyName );
        ObjectMetadata objMetadata = s3Client
                .getObjectMetadata( metadataRequest );
        return objMetadata.getLastModified();
    }
}
