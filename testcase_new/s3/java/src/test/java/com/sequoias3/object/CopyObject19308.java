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
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import java.io.File;
import java.io.IOException;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * @Description seqDB-19308:不同桶指定versionId复制对象
 * @author wuyan
 * @Date 2019.09.17
 * @version 1.00
 */
public class CopyObject19308 extends S3TestBase {
    private AtomicInteger actSuccessTests = new AtomicInteger( 0 );
    private String srcKeyName = "/object19308a";
    private String destKeyName = "/object19308b";
    private String srcBucketName = "bucket19308a";
    private String destBucketName = "bucket19308b";
    private AmazonS3 s3Client = null;
    private int fileSize1 = 1024 * 1024 * 20;
    private int fileSize2 = 1024 * 1024 * 30;
    private File localPath = null;
    private String filePath1 = null;
    private String filePath2 = null;
    private String currentVersionId = "2";
    private String hisVersionId = "0";

    @DataProvider(name = "copyObjectInfoProvider")
    public Object[][] generateCopyObjectInfo() {
        return new Object[][] {
                // the parameter is srcVersionId, copyFileSize,copyFilePath
                // test a: the versionId of copy srcObject is currentVersionId
                new Object[] { currentVersionId, fileSize2, filePath2 },
                // test b: the versionId of copy srcObject is the oldest
                // hisVersionId
                new Object[] { hisVersionId, fileSize1, filePath1 } };
    }

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
        CommLib.clearBucket( s3Client, srcBucketName );
        CommLib.clearBucket( s3Client, destBucketName );

        s3Client.createBucket( srcBucketName );
        s3Client.createBucket( destBucketName );
        CommLib.setBucketVersioning( s3Client, srcBucketName, "Enabled" );

        // put src object ,there are three versions of the object
        s3Client.putObject( srcBucketName, srcKeyName, new File( filePath1 ) );
        s3Client.putObject( srcBucketName, srcKeyName,
                "thefirstVersionContent!" );
        s3Client.putObject( srcBucketName, srcKeyName, new File( filePath2 ) );
    }

    @Test(dataProvider = "copyObjectInfoProvider")
    public void testCopyObject( String versionId, int fileSize,
            String filePath ) throws Exception {
        CopyObjectRequest request = new CopyObjectRequest( srcBucketName,
                srcKeyName, currentVersionId, destBucketName, destKeyName );
        CopyObjectResult result = s3Client.copyObject( request );

        checkObjectAttributeInfo( result, destBucketName, destKeyName,
                fileSize2, filePath2 );
        checkObjectContent( destBucketName, destKeyName, filePath2 );
        actSuccessTests.getAndIncrement();
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( actSuccessTests.get() == generateCopyObjectInfo().length ) {
                CommLib.clearBucket( s3Client, srcBucketName );
                CommLib.clearBucket( s3Client, destBucketName );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            s3Client.shutdown();
        }
    }

    private void checkObjectContent( String bucketName, String keyName,
            String filePath ) throws Exception {
        String downfileMd5 = ObjectUtils.getMd5OfObject( s3Client, localPath,
                bucketName, keyName );
        Assert.assertEquals( downfileMd5, TestTools.getMD5( filePath ) );
    }

    private void checkObjectAttributeInfo( CopyObjectResult objAttrInfo,
            String bucketName, String keyName, int fileSize, String filePath )
            throws IOException {
        String expMd5 = TestTools.getMD5( filePath );
        Assert.assertEquals( objAttrInfo.getETag(), expMd5 );

        // check the attributeInfo of get object
        GetObjectMetadataRequest request = new GetObjectMetadataRequest(
                bucketName, keyName );
        ObjectMetadata result = s3Client.getObjectMetadata( request );
        Assert.assertEquals( result.getETag(), expMd5 );
        Assert.assertEquals( result.getContentLength(), fileSize );
        Assert.assertEquals( result.getVersionId(), "null",
                "the keyName=" + keyName );
    }
}
