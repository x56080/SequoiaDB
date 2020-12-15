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
 * @Description seqDB-15903:different users create buckets of the same name
 * @author wuyan
 * @Date 2018.09.30
 * @version 1.00
 */
public class CreateBucket15903 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket15903";
    private String userName1 = "user15903_a";
    private String userName2 = "user15903_b";
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
    }

    @Test
    public void testCreateBucket() throws Exception {
        s3Client1.createBucket( new CreateBucketRequest( bucketName ) );

        // create buckets of the same name fail by other user
        try {
            s3Client2.createBucket( new CreateBucketRequest( bucketName ) );
            Assert.fail( "create bucket with same name should fail!" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "BucketAlreadyExists" );
        }

        int bucketNum1 = 1;
        int bucketNum2 = 0;
        checkCreateBucketResult( s3Client1, bucketName, userName1, bucketNum1 );
        checkCreateBucketResult( s3Client2, bucketName, userName2, bucketNum2 );
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

    private void checkCreateBucketResult( AmazonS3 s3Client, String bucketName,
            String userName, int bucketNums ) {
        // check bucket nums
        List< Bucket > buckets = s3Client.listBuckets();
        Assert.assertEquals( buckets.size(), bucketNums );

        // create bucket success, than check the bucket name and owner name
        if ( buckets.size() != 0 ) {
            Bucket bucket = buckets.get( 0 );
            String actOwner = bucket.getOwner().getDisplayName();
            String actBucketName = bucket.getName();
            Assert.assertEquals( actBucketName, bucketName );
            Assert.assertEquals( actOwner, userName );
        }
    }

}
