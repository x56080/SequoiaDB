package com.sequoias3.bucket;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
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
 * test content: concurrent delete same bucket testlink-case: seqDB-15918
 *
 * @author wangkexin
 * @Date 2018.10.16
 * @version 1.00
 */
public class CreateBucket15918 extends S3TestBase {
    private final int defaultNums = 30;
    private boolean runSuccess = false;
    private String userName = "user15918";
    private String bucketName = "bucket15918";
    private String delBucketName = "bucket15918.28";
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
    private void testGetBuckets() throws Exception {
        DeleteBucketThread deleteBucket = new DeleteBucketThread(
                delBucketName );
        deleteBucket.start( 10 );
        Assert.assertTrue( deleteBucket.isSuccess(),
                deleteBucket.getErrorMsg() );

        checkBucketResult( s3Client );
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

    private void createBuckets( AmazonS3 s3Client ) {
        for ( int i = 0; i < defaultNums; i++ ) {
            String subBucketName = bucketName + "." + i;
            s3Client.createBucket( subBucketName );
        }
    }

    private void checkBucketResult( AmazonS3 s3Client ) {
        // check bucket nums
        List<Bucket> buckets = s3Client.listBuckets();
        Assert.assertEquals( buckets.size(), defaultNums - 1 );

        for ( int i = 0; i < buckets.size(); i++ ) {
            Bucket bucket = buckets.get( i );
            String actBucketName = bucket.getName();
            Assert.assertNotEquals( actBucketName, delBucketName,
                    "the bucket still exist!" );
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
                s3Client.deleteBucket( bucketName );
            } catch ( AmazonS3Exception e ) {
                Assert.assertEquals( e.getErrorCode(), "NoSuchBucket" );
            } finally {
                if ( s3Client != null ) {
                    s3Client.shutdown();
                }
            }
        }
    }
}
