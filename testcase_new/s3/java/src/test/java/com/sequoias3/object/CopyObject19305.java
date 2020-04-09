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
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import java.io.File;
import java.io.IOException;
import java.util.Date;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * @Description seqDB-19305:不同桶复制对象
 * @author wuyan
 * @Date 2019.09.17
 * @version 1.00
 */
public class CopyObject19305 extends S3TestBase {
    private AtomicInteger actSuccessTests = new AtomicInteger( 0 );
    private String keyName = "aa%maa/bb%object19305";
    private String bucketNameA = "bucket19305a";
    private String bucketNameB = "bucket19305b";
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * 1024 * 30;
    private File localPath = null;
    private String filePath = null;
    private Date expModifiedTime;

    @DataProvider(name = "keyNameProvider", parallel = true)
    public Object[][] generateKeyName() {
        return new Object[][] {
                // the parameter is destKeyName
                // test a: destKeyName is the same as source keyName
                new Object[] { keyName },
                // test b: destKeyName is different from source keyName
                new Object[] { "/destKey/19305.txt" } };
    }

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
        CommLib.clearBucket( s3Client, bucketNameA );
        CommLib.clearBucket( s3Client, bucketNameB );

        s3Client.createBucket( bucketNameA );
        s3Client.createBucket( bucketNameB );
        s3Client.putObject( bucketNameA, keyName, new File( filePath ) );
        expModifiedTime = s3Client.getObject( bucketNameA, keyName )
                .getObjectMetadata().getLastModified();
    }

    @Test(dataProvider = "keyNameProvider")
    public void testCopyObject( String destKeyName ) throws Exception {
        CopyObjectResult result = s3Client.copyObject( bucketNameA, keyName,
                bucketNameB, destKeyName );

        checkObjectAttributeInfo( result, bucketNameB, destKeyName,
                expModifiedTime );
        checkObjectContent( bucketNameB, destKeyName );
        checkObjectContent( bucketNameA, keyName );
        actSuccessTests.getAndIncrement();
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( actSuccessTests.get() == generateKeyName().length ) {
                CommLib.clearBucket( s3Client, bucketNameA );
                CommLib.clearBucket( s3Client, bucketNameB );
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
            String bucketName, String keyName, Date beforeDate )
            throws IOException {
        String expMd5 = TestTools.getMD5( filePath );
        Assert.assertEquals( objAttrInfo.getETag(), expMd5 );

        // check the attributeInfo of get object
        GetObjectMetadataRequest request = new GetObjectMetadataRequest(
                bucketName, keyName );
        ObjectMetadata result = s3Client.getObjectMetadata( request );
        Date modifiedDate = result.getLastModified();
        String defaultContenttype = "text/plain";
        Assert.assertEquals( result.getContentType(), defaultContenttype );
        Assert.assertEquals( result.getETag(), expMd5 );
        Assert.assertEquals( result.getContentLength(), fileSize );

        Assert.assertTrue(
                modifiedDate.after( new Date( beforeDate.getTime() - 1000 ) ),
                "modifiedDate must greater beforeDate, modifiedDate = "
                        + modifiedDate + ", beforeDate = " + beforeDate );
        // 60s error allowed in time range
        Assert.assertTrue(
                modifiedDate.getTime() - beforeDate.getTime() < 60 * 1000,
                "modifiedDate = " + modifiedDate + ", " + beforeDate );
    }
}
