package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.IOException;

/**
 * test content: 指定桶不存在 testlink-case: seqDB-16348
 *
 * @author wangkexin
 * @Date 2018.11.13
 * @version 1.00
 */
public class CreateObject16348 extends S3TestBase {
    private boolean runSuccess = false;
    private String non_existent_bucketName = "bucket16348";
    private String keyName = "/aa/bb/object16348.png";
    private AmazonS3 s3Client = null;
    private String expContent = "file16348";

    @BeforeClass
    private void setUp() throws IOException {
        s3Client = CommLib.buildS3Client();
    }

    @Test
    public void testPutObject() throws Exception {
        // put object in a nonexistent bucket.
        try {
            s3Client.putObject( non_existent_bucketName, keyName, expContent );
            Assert.fail( "exp fail but found success" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "NoSuchBucket" );
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        if ( runSuccess ) {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }
}
