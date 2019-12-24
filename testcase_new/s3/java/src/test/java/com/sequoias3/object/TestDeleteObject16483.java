package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.DeleteObjectRequest;
import com.amazonaws.services.s3.model.DeleteVersionRequest;
import com.sequoiadb.exception.BaseException;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * test content: DeleteObject接口参数校验 testlink-case: seqDB-16483
 *
 * @author wangkexin
 * @Date 2019.01.08
 * @version 1.00
 */
public class TestDeleteObject16483 extends S3TestBase {
    private String bucketName = "bucket16483";
    private String keyName = "key16483";
    private AmazonS3 s3Client = null;
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
        s3Client.putObject( bucketName, keyName, "testcontent16483" );
    }

    @Test
    public void testDeleteObject() throws Exception {
        // test a : deleteObject 合法值校验
        s3Client.deleteObject( new DeleteObjectRequest( bucketName, keyName ) );
        Assert.assertFalse( s3Client.doesObjectExist( bucketName, keyName ) );

        // test b : deleteObject 非法值校验 取值为null
        try {
            s3Client.deleteObject( null );
            Assert.fail( "when delete object request is null, is should fail" );
        } catch ( IllegalArgumentException e ) {
            Assert.assertEquals( e.getMessage(),
                    "The delete object request must be specified when deleting an object" );
        }

        // test c : deleteVersion 合法值校验
        s3Client.putObject( bucketName, keyName, "testcontent16483" );
        s3Client.deleteVersion(
                new DeleteVersionRequest( bucketName, keyName, "null" ) );
        Assert.assertFalse( s3Client.doesObjectExist( bucketName, keyName ) );

        try {
            s3Client.deleteVersion( null );
            Assert.fail(
                    "when delete version request is null, is should fail" );
        } catch ( IllegalArgumentException e ) {
            Assert.assertEquals( e.getMessage(),
                    "The delete version request object must be specified when deleting a version" );
        }

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                CommLib.deleteAllObjectVersions( s3Client, bucketName );
                s3Client.deleteBucket( bucketName );
            }
        } catch ( BaseException e ) {
            Assert.fail( "clean up failed:" + e.getMessage() );
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }
}
