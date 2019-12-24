package com.sequoias3.bucket;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.Bucket;
import com.sequoiadb.exception.BaseException;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.S3ThreadBase;
import com.sequoias3.testcommon.s3utils.UserUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.List;

/**
 * test content: concurrent create and delete bucket testlink-case: seqDB-15920
 *
 * @author wangkexin
 * @Date 2018.10.16
 * @version 1.00
 */
public class CreateBucket15920 extends S3TestBase {
    private boolean runSuccess = false;
    private String userName = "user15920";
    private String bucketName = "bucket15920";
    private String roleName = "normal";
    private AmazonS3 s3Client = null;
    private String[] acessKeys = null;

    @BeforeClass
    private void setUp() throws Exception {
        CommLib.clearUser( userName );
        acessKeys = UserUtils.createUser( userName, roleName );
        s3Client = CommLib.buildS3Client( acessKeys[ 0 ], acessKeys[ 1 ] );
    }

    @Test
    private void testCreateBucket() throws Exception {
        CreateBucketThread createBucket = new CreateBucketThread( bucketName );
        DeleteBucketThread deleteBucket = new DeleteBucketThread( bucketName );

        createBucket.start();
        deleteBucket.start();

        Assert.assertTrue( createBucket.isSuccess(),
                createBucket.getErrorMsg() );

        if ( deleteBucket.isSuccess() ) {
            checkBucketResult( s3Client, 0 );
        } else {
            checkBucketResult( s3Client, 1 );
        }
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( runSuccess ) {
                UserUtils.deleteUser( userName );
            }
        } catch ( BaseException e ) {
            Assert.fail( "clean up failed:" + e.getMessage() );
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }

    private void checkBucketResult( AmazonS3 s3Client, int bucketsize ) {
        List<Bucket> buckets = s3Client.listBuckets();
        Assert.assertEquals( buckets.size(), bucketsize );
        if ( buckets.size() > 0 ) {
            Bucket bucket = buckets.get( 0 );
            String actBucketName = bucket.getName();
            String actOwner = bucket.getOwner().getDisplayName();
            Assert.assertEquals( actBucketName, bucketName );
            Assert.assertEquals( actOwner, userName );
        }
    }

    private class CreateBucketThread extends S3ThreadBase {
        String bucketName;

        public CreateBucketThread( String bucketName ) {
            this.bucketName = bucketName;
        }

        @Override
        public void exec() throws Exception {
            AmazonS3 s3Client = CommLib
                    .buildS3Client( acessKeys[ 0 ], acessKeys[ 1 ] );
            try {
                s3Client.createBucket( bucketName );
            } finally {
                if ( s3Client != null ) {
                    s3Client.shutdown();
                }
            }
        }
    }

    private class DeleteBucketThread extends S3ThreadBase {
        String bucketName;

        public DeleteBucketThread( String bucketName ) {
            this.bucketName = bucketName;
        }

        @Override
        public void exec() throws Exception {
            AmazonS3 s3Client = CommLib
                    .buildS3Client( acessKeys[ 0 ], acessKeys[ 1 ] );
            try {
                Thread.sleep( 2 );
                s3Client.deleteBucket( bucketName );
            } finally {
                if ( s3Client != null ) {
                    s3Client.shutdown();
                }
            }
        }
    }
}
