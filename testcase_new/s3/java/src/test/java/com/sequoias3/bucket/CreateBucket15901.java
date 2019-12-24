package com.sequoias3.bucket;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.Bucket;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.Owner;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.List;

/**
 * test content: create bucket testlink-case: seqDB-15901
 *
 * @author wuyan
 * @Date 2018.09.28
 * @version 1.00
 */
public class CreateBucket15901 extends S3TestBase {
    String bucketName = "bucket15901";
    private boolean runSuccess = false;
    private AmazonS3 s3Client = null;

    @SuppressWarnings("deprecation")
    @BeforeClass
    private void setUp() {
        s3Client = CommLib.buildS3Client();
        if ( s3Client.doesBucketExist( bucketName ) ) {
            s3Client.deleteBucket( bucketName );
        }
    }

    @Test
    public void testCreateBucket() {
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );
        checkCreateBucketResult();
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                s3Client.deleteBucket( bucketName );
            }
        } finally {
            s3Client.shutdown();
        }
    }

    private void checkCreateBucketResult() {
        // create one bucket,check the bucket name and owner name
        List<Bucket> buckets = s3Client.listBuckets();
        boolean findBucketFlag = false;
        for ( int i = 0; i < buckets.size(); i++ ) {
            String actBucketName = buckets.get( i ).getName();
            // get the create bucket,then check the bucket name and owner
            if ( actBucketName.equals( bucketName ) ) {
                Owner actOwner = buckets.get( i ).getOwner();
                Assert.assertEquals( actOwner.getDisplayName(),
                        S3TestBase.s3UserName );
                findBucketFlag = true;
                break;
            }
        }
        Assert.assertTrue( findBucketFlag, " the bucket must be exist!" );
    }

}
