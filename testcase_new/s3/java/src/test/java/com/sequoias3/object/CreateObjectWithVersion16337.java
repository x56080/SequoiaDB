package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.GetObjectMetadataRequest;
import com.amazonaws.services.s3.model.ObjectMetadata;
import com.amazonaws.services.s3.model.PutObjectResult;
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
import java.util.Date;

/**
 * @Description seqDB-16337:suspended bucket versioning , create object on the
 *              bucket
 * @author wuyan
 * @Date 2018.11.6
 * @version 1.00
 */
public class CreateObjectWithVersion16337 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket16337";
    private String keyName = "object16337";
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * 1024;
    private File localPath = null;
    private String filePath = null;

    @BeforeClass
    private void setUp() throws IOException {
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
        CommLib.setBucketVersioning( s3Client, bucketName, "Suspended" );
    }

    @Test
    public void testCreateObject() throws Exception {
        long currentTime = new Date().getTime();
        // current time 1 seccond earlier to reduce acquisition error
        Date beforeDate = new Date( currentTime - 1000 );
        PutObjectResult result = s3Client
                .putObject( bucketName, keyName, new File( filePath ) );
        checkObjectAttributeInfo( result, beforeDate );
        checkPutObjectResult( bucketName );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                ObjectUtils.deleteObjectAllVersions( s3Client, bucketName,
                        keyName );
                s3Client.deleteBucket( bucketName );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }

    private void checkPutObjectResult( String bucketName ) throws Exception {
        // down file
        String downfileMd5 = ObjectUtils
                .getMd5OfObject( s3Client, localPath, bucketName, keyName );
        Assert.assertEquals( downfileMd5, TestTools.getMD5( filePath ) );
    }

    private void checkObjectAttributeInfo( PutObjectResult objAttrInfo,
            Date beforeDate ) throws IOException {
        Date afterDate = new Date();
        String expMd5 = TestTools.getMD5( filePath );
        Assert.assertEquals( objAttrInfo.getETag(), expMd5 );
        // suspended versiong the versionId is null
        Assert.assertEquals( objAttrInfo.getVersionId(), "null" );

        // check the attributeInfo of get object
        GetObjectMetadataRequest request = new GetObjectMetadataRequest(
                bucketName, keyName );
        ObjectMetadata result = s3Client.getObjectMetadata( request );
        Date modifiedDate = result.getLastModified();
        Assert.assertEquals( result.getVersionId(), "null" );
        Assert.assertEquals( result.getETag(), expMd5 );
        Assert.assertEquals( result.getContentLength(), fileSize );
        // modifiedDate range is [ beforeDate, afterDate]
        Assert.assertFalse( modifiedDate.before( beforeDate ),
                "modifiedDate must not be less than beforeDate,"
                        + "modifiedDate:" + modifiedDate + " beforeDate:"
                        + beforeDate );
        Assert.assertFalse( modifiedDate.after( afterDate ),
                "modifiedDate must not be greater than afterDate,"
                        + "modifiedDate:" + modifiedDate + " afterDate:"
                        + afterDate );
    }
}
