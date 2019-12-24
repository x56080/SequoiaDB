package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.GetObjectRequest;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.IOException;

/**
 * test content: enabling bucket versioning,get the object key does not exist.
 * test 16365:Do not specify versionId test 16366:specify versionId
 * testlink-case: seqDB-16365/16366
 *
 * @author wuyan
 * @Date 2018.11.14
 * @version 1.00
 */
public class GetObjectNoExist16365_16366 extends S3TestBase {
    private boolean runSuccess = false;
    private String keya = "aa/bb/object16365";
    private String keyb = "aa/bb/object16366";
    private AmazonS3 s3Client = null;

    @BeforeClass
    private void setUp() throws IOException {
        s3Client = CommLib.buildS3Client();
        ObjectUtils.deleteObjectAllVersions( s3Client,
                S3TestBase.enableVerBucketName, keya );
    }

    @Test
    public void testGetObject() throws Exception {
        // test 16365:Do not specify versionId
        try {
            s3Client.getObject( S3TestBase.enableVerBucketName, keya );
            Assert.fail( "get not exist key must be fail !" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "NoSuchKey" );
        }

        // test 16366: specify versionId
        s3Client.putObject( S3TestBase.enableVerBucketName, keya,
                "test object" );
        try {
            GetObjectRequest request = new GetObjectRequest(
                    S3TestBase.bucketName, keyb, "0" );
            s3Client.getObject( request );
            Assert.fail( "specify versionId get not exist key must be fail !" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "NoSuchVersion" );
        }
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                ObjectUtils.deleteObjectAllVersions( s3Client,
                        S3TestBase.enableVerBucketName, keya );
            }
        } finally {
            s3Client.shutdown();
        }
    }

}
