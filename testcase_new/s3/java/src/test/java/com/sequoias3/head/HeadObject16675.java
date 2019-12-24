package com.sequoias3.head;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.GetObjectMetadataRequest;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.UserUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.IOException;

/**
 * @Description seqDB-16675: head object by the other user
 * @author wuyan
 * @Date 2018.12.17
 * @version 1.00
 */
public class HeadObject16675 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket16675";
    private String key = "test/object16675";
    private String userName1 = "user16675_a";
    private String userName2 = "user16675_b";
    private AmazonS3 s3Client1 = null;
    private AmazonS3 s3Client2 = null;
    private String roleName = "normal";

    @BeforeClass
    private void setUp() throws IOException {
        CommLib.clearUser( userName1 );
        CommLib.clearUser( userName2 );
        String[] acessKeys1 = UserUtils.createUser( userName1, roleName );
        String[] acessKeys2 = UserUtils.createUser( userName2, roleName );
        s3Client1 = CommLib.buildS3Client( acessKeys1[ 0 ], acessKeys1[ 1 ] );
        s3Client2 = CommLib.buildS3Client( acessKeys2[ 0 ], acessKeys2[ 1 ] );
        s3Client2.createBucket( bucketName );
    }

    @Test
    public void testHeadBucket() throws IOException {
        s3Client2.putObject( bucketName, key, "testbucketObject16675!" );
        GetObjectMetadataRequest request = new GetObjectMetadataRequest(
                bucketName, key );

        try {
            s3Client1.getObjectMetadata( request );
            Assert.fail( "head object must be fail!" );
        } catch ( AmazonS3Exception e ) {
            // return 403 Forbidden
            Assert.assertEquals( e.getStatusCode(), 403 );
        }
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                UserUtils.deleteUser( userName1 );
                UserUtils.deleteUser( userName2 );
            }
        } finally {
            s3Client1.shutdown();
            s3Client2.shutdown();
        }
    }
}
