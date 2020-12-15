package com.sequoias3.bucket;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.Bucket;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.ObjectMetadata;
import com.amazonaws.services.s3.model.PutObjectResult;
import com.amazonaws.services.s3.model.S3Object;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.UserUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.List;

/**
 * @Description seqDB-15902:create bucket by different ownersB *
 * @author wuyan
 * @Date 2018.09.28
 * @version 1.00
 */
public class CreateBucket15902 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName1 = "bucket15902a";
    private String bucketName2 = "bucket15902b";
    private String key = "testkey15902";
    private String userName1 = "user15902_a";
    private String userName2 = "user15902_b";
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
        s3Client1.createBucket( new CreateBucketRequest( bucketName1 ) );
        s3Client2.createBucket( new CreateBucketRequest( bucketName2 ) );
        checkCreateBucketResult( s3Client1, bucketName1, userName1 );
        checkCreateBucketResult( s3Client2, bucketName2, userName2 );
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
            String userName ) {
        // create one bucket,check the bucket name and owner name
        List< Bucket > buckets = s3Client.listBuckets();
        Assert.assertEquals( buckets.size(), 1, " only one bucket" );
        Bucket bucket = buckets.get( 0 );
        String actOwner = bucket.getOwner().getDisplayName();
        String actBucketName = bucket.getName();
        Assert.assertEquals( actBucketName, bucketName );
        Assert.assertEquals( actOwner, userName );

        // put object in the bucket
        String content = "testbucket";
        PutObjectResult result = s3Client.putObject( bucketName, key, content );
        String rEtag = result.getETag();
        S3Object object = s3Client.getObject( bucketName, key );
        ObjectMetadata metadata = object.getObjectMetadata();
        String gEtag = metadata.getETag();
        Assert.assertEquals( gEtag, rEtag );
    }

}
