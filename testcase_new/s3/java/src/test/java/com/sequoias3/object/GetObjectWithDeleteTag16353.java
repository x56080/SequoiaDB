package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.IOException;

/**
 * @Description seqDB-16353: get the object of delete tag
 * @author wuyan
 * @Date 2018.11.6
 * @version 1.00
 */
public class GetObjectWithDeleteTag16353 extends S3TestBase {
    private boolean runSuccess = false;
    private String key = "object16353";
    private AmazonS3 s3Client = null;

    @BeforeClass
    private void setUp() throws IOException {
        s3Client = CommLib.buildS3Client();
    }

    @Test
    public void testGetObject() throws Exception {
        s3Client.deleteObject( S3TestBase.enableVerBucketName, key );
        try {
            s3Client.getObject( bucketName, key );
            Assert.fail( "get delete tag object must be fail!" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "NoSuchKey" );
        }
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                ObjectUtils.deleteObjectAllVersions( s3Client,
                        S3TestBase.enableVerBucketName, key );
            }
        } finally {
            s3Client.shutdown();
        }
    }

}
