package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Description: 指定桶不存在
 *
 * @author wangkexin
 * @Date 2018.11.28
 * @version 1.00
 */

public class DeleteObject16456 extends S3TestBase {
    private String bucketName = "bucket16456";
    private String keyName = "testkey16456";
    private String file = "object16456";
    private AmazonS3 s3Client = null;
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {
        s3Client = CommLib.buildS3Client();
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );
        s3Client.putObject( bucketName, keyName, file );
    }

    @Test
    public void testGetObjectList() throws Exception {
        // delete object with nonexistent bucket name
        try {
            s3Client.deleteObject( "nonexistentbucket16456", keyName );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "NoSuchBucket" );
        }

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        if ( runSuccess ) {
            CommLib.deleteAllObjectVersions( s3Client, bucketName );
            s3Client.deleteBucket( bucketName );
        }
    }
}
