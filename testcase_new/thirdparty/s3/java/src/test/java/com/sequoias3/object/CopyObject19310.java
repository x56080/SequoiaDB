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
 * @Description seqDB-19310: 不同桶复制对象，指定源桶禁用版本控制
 * @author wuyan
 * @Date 2019.09.17
 * @version 1.00
 */
public class CopyObject19310 extends S3TestBase {
    private boolean runSuccess = false;
    private String srcKeyName = "/object19310a";
    private String destKeyName = "/object19310b";
    private String srcBucketName = "bucket19310a";
    private String destBucketName = "bucket19310b";
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * 1024 * 10;
    private File localPath = null;
    private String filePath = null;

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator
                + TestTools.getClassName() );
        filePath = localPath + File.separator + "localFile_" + fileSize
                + ".txt";

        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath, fileSize );
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, srcBucketName );
        CommLib.clearBucket( s3Client, destBucketName );

        s3Client.createBucket( srcBucketName );
        CommLib.setBucketVersioning( s3Client, srcBucketName, "Enabled" );
        s3Client.createBucket( destBucketName );

        s3Client.putObject( srcBucketName, srcKeyName,
                "contentofhistoryVersion" );
        s3Client.putObject( srcBucketName, srcKeyName,
                "contentofcurrentVersion" );
        CommLib.setBucketVersioning( s3Client, srcBucketName, "Suspended" );
        s3Client.putObject( srcBucketName, srcKeyName, new File( filePath ) );
    }

    @Test
    public void testCopyObject() throws Exception {
        CopyObjectResult result = s3Client.copyObject( srcBucketName,
                srcKeyName, destBucketName, destKeyName );

        checkObjectAttributeInfo( result, destBucketName, destKeyName );
        checkObjectContent( destBucketName, destKeyName );

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

    private void checkObjectContent( String bucketName, String keyName )
            throws Exception {
        // down file
        String downfileMd5 = ObjectUtils.getMd5OfObject( s3Client, localPath,
                bucketName, keyName );
        Assert.assertEquals( downfileMd5, TestTools.getMD5( filePath ) );
    }

    private void checkObjectAttributeInfo( CopyObjectResult objAttrInfo,
            String bucketName, String keyName ) throws IOException {
        String expMd5 = TestTools.getMD5( filePath );
        Assert.assertEquals( objAttrInfo.getETag(), expMd5 );

        // check the attributeInfo of get object
        GetObjectMetadataRequest request = new GetObjectMetadataRequest(
                bucketName, keyName );
        ObjectMetadata result = s3Client.getObjectMetadata( request );

        Assert.assertEquals( result.getETag(), expMd5 );
        Assert.assertEquals( result.getContentLength(), fileSize );
        Assert.assertEquals( result.getVersionId(), "null" );
    }
}
