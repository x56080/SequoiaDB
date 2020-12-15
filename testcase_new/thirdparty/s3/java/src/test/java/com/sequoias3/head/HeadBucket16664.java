package com.sequoias3.head;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.HeadBucketRequest;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.UserUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Description seqDB-16664: head bucket by the other user
 * @author wuyan
 * @Date 2018.09.28
 * @version 1.00
 */
public class HeadBucket16664 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket16664";
    private String userName1 = "user16664_a";
    private String userName2 = "user16664_b";
    private AmazonS3 s3Client1 = null;
    private AmazonS3 s3Client2 = null;
    private String roleName = "normal";

    @BeforeClass
    private void setUp() {
        CommLib.clearUser( userName1 );
        CommLib.clearUser( userName2 );
        String[] acessKeys1 = UserUtils.createUser( userName1, roleName );
        String[] acessKeys2 = UserUtils.createUser( userName2, roleName );
        s3Client1 = CommLib.buildS3Client( acessKeys1[ 0 ], acessKeys1[ 1 ] );
        s3Client2 = CommLib.buildS3Client( acessKeys2[ 0 ], acessKeys2[ 1 ] );
    }

    @Test
    public void testCreateBucket() {
        s3Client1.createBucket( new CreateBucketRequest( bucketName ) );
        HeadBucketRequest request = new HeadBucketRequest( bucketName );
        s3Client1.headBucket( request );

        try {
            s3Client2.headBucket( request );
            Assert.fail( "head bucket must be fail!" );
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
