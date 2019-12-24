package com.sequoias3.head;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.GetObjectMetadataRequest;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.IOException;

/**
 * @Description seqDB-16676: head object and specify the bucket does not exist.
 * @author wuyan
 * @Date 2018.12.17
 * @version 1.00
 */
public class HeadObject16676 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket16676";
    private String key = "test/test2/object16676";
    private AmazonS3 s3Client = null;

    @BeforeClass
    private void setUp() throws IOException {
        s3Client = CommLib.buildS3Client();
        s3Client.putObject( S3TestBase.bucketName, key,
                "testbucketObject16676!" );
    }

    @Test
    public void testCreateBucket() {
        GetObjectMetadataRequest request = new GetObjectMetadataRequest(
                bucketName, key );
        try {
            s3Client.getObjectMetadata( request );
            Assert.fail( "head object must be fail!" );
        } catch ( AmazonS3Exception e ) {
            // return 404 Not found
            Assert.assertEquals( e.getStatusCode(), 404 );
        }
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                s3Client.deleteObject( S3TestBase.bucketName, key );
            }
        } finally {
            s3Client.shutdown();
        }
    }
}
