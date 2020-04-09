package com.sequoias3.bucket;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.Bucket;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.UserUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.List;

/**
 * @Description seqDB-15917:delete bucket by different owners *
 * @author wuyan
 * @Date 2018.10.10
 * @version 1.00
 */
public class DeleteBucket15917 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName1 = "bucket15917a";
    private String bucketName2 = "bucket15917b";
    private String userName1 = "user15917_a";
    private String userName2 = "user15917_b";
    private String roleName = "normal";
    private AmazonS3 s3Client1 = null;
    private AmazonS3 s3Client2 = null;

    @BeforeClass
    private void setUp() throws Exception {
        CommLib.clearUser( userName1 );
        CommLib.clearUser( userName2 );
        String[] acessKeys1 = UserUtils.createUser( userName1, roleName );
        String[] acessKeys2 = UserUtils.createUser( userName2, roleName );
        s3Client1 = CommLib.buildS3Client( acessKeys1[ 0 ], acessKeys1[ 1 ] );
        s3Client2 = CommLib.buildS3Client( acessKeys2[ 0 ], acessKeys2[ 1 ] );

        s3Client1.createBucket( new CreateBucketRequest( bucketName1 ) );
        s3Client2.createBucket( new CreateBucketRequest( bucketName2 ) );
    }

    @Test
    public void testCreateBucket() throws Exception {
        try {
            s3Client1.deleteBucket( bucketName2 );
            Assert.fail( "delete bucket must be fail !" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "AccessDenied" );
        }

        checkResult( s3Client2, bucketName2, userName2 );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
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

    private void checkResult( AmazonS3 s3Client, String bucketName,
            String userName ) {
        // create one bucket,check the bucket name and owner name
        List< Bucket > buckets = s3Client.listBuckets();
        Assert.assertEquals( buckets.size(), 1, " only one bucket" );
        Bucket bucket = buckets.get( 0 );
        String actOwner = bucket.getOwner().getDisplayName();
        String actBucketName = bucket.getName();
        Assert.assertEquals( actBucketName, bucketName );
        Assert.assertEquals( actOwner, userName );
    }

}
