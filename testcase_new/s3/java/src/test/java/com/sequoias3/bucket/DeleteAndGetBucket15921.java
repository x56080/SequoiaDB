package com.sequoias3.bucket;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.Bucket;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.S3ThreadBase;
import com.sequoias3.testcommon.s3utils.UserUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/**
 * @Description seqDB-15921:concurrent delete bucket and get bucketlist
 * @author wuyan
 * @Date 2018.10.10
 * @version 1.00
 */
public class DeleteAndGetBucket15921 extends S3TestBase {
    private final int defaultNums = 30;
    private boolean runSuccess = false;
    private String bucketName = "bucket15921";
    private String userName = "user15921";
    private String roleName = "normal";
    private AmazonS3 s3Client = null;
    private String[] acessKeys = null;

    @BeforeClass
    private void setUp() throws Exception {
        CommLib.clearUser( userName );
        acessKeys = UserUtils.createUser( userName, roleName );
        s3Client = CommLib.buildS3Client( acessKeys[ 0 ], acessKeys[ 1 ] );
        createBuckets( s3Client );
    }

    @Test
    public void testCreateBucket() throws Exception {
        List< DeleteBucketThread > deleteBuckets = new ArrayList<>( 20 );
        GetBucketThread getBuckets = new GetBucketThread();

        List< String > existBuckets = new ArrayList< String >();
        for ( int i = 0; i < defaultNums; i++ ) {
            String subBucketName = bucketName + "." + i;
            if ( i % 2 == 0 ) {
                deleteBuckets.add( new DeleteBucketThread( subBucketName ) );
            } else {
                existBuckets.add( subBucketName );
            }
        }

        for ( DeleteBucketThread deleteBucket : deleteBuckets ) {
            deleteBucket.start();
        }
        getBuckets.start();

        for ( DeleteBucketThread deleteBucket : deleteBuckets ) {
            Assert.assertTrue( deleteBucket.isSuccess(),
                    deleteBucket.getErrorMsg() );
        }
        Assert.assertTrue( getBuckets.isSuccess(), getBuckets.getErrorMsg() );

        checkDeleteBucketResult( s3Client, existBuckets );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( runSuccess ) {
                UserUtils.deleteUser( userName );
            }
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }

    private void createBuckets( AmazonS3 s3Client ) {
        for ( int i = 0; i < defaultNums; i++ ) {
            String subBucketName = bucketName + "." + i;
            s3Client.createBucket( subBucketName );
        }
    }

    private void checkDeleteBucketResult( AmazonS3 s3Client,
            List< String > existBuckets ) {
        // check bucket nums
        List< Bucket > buckets = s3Client.listBuckets();
        Assert.assertEquals( buckets.size(), defaultNums / 2 );

        List< String > actExistBuckets = new ArrayList< String >();
        for ( int i = 0; i < buckets.size(); i++ ) {
            Bucket bucket = buckets.get( i );
            String actBucketName = bucket.getName();
            actExistBuckets.add( actBucketName );
        }

        Collections.sort( actExistBuckets );
        Collections.sort( existBuckets );
        Assert.assertEquals( actExistBuckets, existBuckets, "buckets actual:"
                + actExistBuckets + ";the expect :" + existBuckets );
    }

    private class DeleteBucketThread extends S3ThreadBase {
        String bucketName;

        public DeleteBucketThread( String bucketName ) {
            this.bucketName = bucketName;
        }

        @Override
        public void exec() throws Exception {
            AmazonS3 s3Client = CommLib.buildS3Client( acessKeys[ 0 ],
                    acessKeys[ 1 ] );
            try {
                s3Client.deleteBucket( bucketName );
            } finally {
                if ( s3Client != null ) {
                    s3Client.shutdown();
                }
            }
        }
    }

    private class GetBucketThread extends S3ThreadBase {
        @Override
        public void exec() throws Exception {
            AmazonS3 s3Client = CommLib.buildS3Client( acessKeys[ 0 ],
                    acessKeys[ 1 ] );
            try {
                List< Bucket > buckets = s3Client.listBuckets();

                // test list bucket success, the buckets not 0
                Assert.assertNotEquals( buckets.size(), 0 );
            } finally {
                if ( s3Client != null ) {
                    s3Client.shutdown();
                }
            }
        }
    }
}
