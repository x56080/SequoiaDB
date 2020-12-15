package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CopyObjectResult;
import com.amazonaws.services.s3.model.GetObjectMetadataRequest;
import com.amazonaws.services.s3.model.ObjectMetadata;
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

/**
 * @Description seqDB-19313:不同桶复制对象，指定目标桶开启版本控制，且指定目标对象已存在
 * @author wuyan
 * @Date 2019.09.18
 * @version 1.00
 */
public class CopyObject19313 extends S3TestBase {
    private boolean runSuccess = false;
    private String srcKeyName = "/srcObject19313";
    private String destKeyName = "/dest/object19313";
    private String srcBucketName = "srcbucket19313";
    private String destBucketName = "destbucket19313";
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * 1024 * 50;
    private int copyFileSize = 1024 * 1024 * 30;
    private File localPath = null;
    private String filePath = null;
    private String copyFilePath = null;

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator
                + TestTools.getClassName() );
        filePath = localPath + File.separator + "localFile_" + fileSize
                + ".txt";
        copyFilePath = localPath + File.separator + "localFile_" + copyFileSize
                + ".txt";

        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath, fileSize );
        TestTools.LocalFile.createFile( copyFilePath, copyFileSize );
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, srcBucketName );
        CommLib.clearBucket( s3Client, destBucketName );

        s3Client.createBucket( srcBucketName );
        s3Client.createBucket( destBucketName );
        CommLib.setBucketVersioning( s3Client, destBucketName, "Enabled" );

        s3Client.putObject( srcBucketName, srcKeyName,
                new File( copyFilePath ) );
        s3Client.putObject( destBucketName, destKeyName, new File( filePath ) );
    }

    @Test
    public void testCopyObject() throws Exception {
        CopyObjectResult result = s3Client.copyObject( srcBucketName,
                srcKeyName, destBucketName, destKeyName );

        String currentVersionId = "1";
        String hisVersionId = "0";
        checkObjectAttributeInfo( result, destBucketName, destKeyName,
                currentVersionId, hisVersionId );
        checkObjectContent( destBucketName, destKeyName, currentVersionId,
                hisVersionId );

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                CommLib.clearBucket( s3Client, srcBucketName );
                CommLib.clearBucket( s3Client, destBucketName );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            s3Client.shutdown();
        }
    }

    private void checkObjectContent( String bucketName, String keyName,
            String currentVersionId, String hisVersionId ) throws Exception {
        // down file by currentVersion
        String downfileMd5 = ObjectUtils.getMd5OfObject( s3Client, localPath,
                bucketName, keyName, currentVersionId );
        Assert.assertEquals( downfileMd5, TestTools.getMD5( copyFilePath ) );

        // down file by historyVersion,is the old keyName
        downfileMd5 = ObjectUtils.getMd5OfObject( s3Client, localPath,
                bucketName, keyName, hisVersionId );
        Assert.assertEquals( downfileMd5, TestTools.getMD5( filePath ) );
    }

    private void checkObjectAttributeInfo( CopyObjectResult objAttrInfo,
            String bucketName, String keyName, String currentVersionId,
            String hisVersionId ) throws IOException {
        String expMd5 = TestTools.getMD5( copyFilePath );
        Assert.assertEquals( objAttrInfo.getETag(), expMd5 );

        // check the attributeInfo of get copy object
        GetObjectMetadataRequest request = new GetObjectMetadataRequest(
                bucketName, keyName );
        ObjectMetadata result = s3Client.getObjectMetadata( request );
        Assert.assertEquals( result.getETag(), expMd5 );
        Assert.assertEquals( result.getContentLength(), copyFileSize );
        Assert.assertEquals( result.getVersionId(), currentVersionId,
                "the keyName=" + keyName );

        // check the attributeInfo of old destObject
        GetObjectMetadataRequest request1 = new GetObjectMetadataRequest(
                bucketName, keyName, hisVersionId );
        ObjectMetadata result1 = s3Client.getObjectMetadata( request1 );
        Assert.assertEquals( result1.getETag(), TestTools.getMD5( filePath ) );
        Assert.assertEquals( result1.getContentLength(), fileSize );
        Assert.assertEquals( result1.getVersionId(), hisVersionId,
                "the keyName=" + keyName );
    }
}
