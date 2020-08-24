package com.sequoias3.head;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.HeadBucketRequest;
import com.sequoiadb.exception.BaseException;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.S3ThreadBase;
import com.sequoias3.testcommon.s3utils.UserUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Description: 并发head查询和删除相同桶 testlink-case: seqDB-16667
 *
 * @author wangkexin
 * @Date 2018.10.16
 * @version 1.00
 */
public class TestHeadBucket16667 extends S3TestBase {
    private boolean runSuccess = false;
    private String userName = "user16667";
    private String bucketName = "bucket16667";
    private String roleName = "normal";
    private AmazonS3 s3Client = null;
    private String[] acessKeys = null;

    @BeforeClass
    private void setUp() throws Exception {
        CommLib.clearUser( userName );
        acessKeys = UserUtils.createUser( userName, roleName );
        s3Client = CommLib.buildS3Client( acessKeys[ 0 ], acessKeys[ 1 ] );
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );
    }

    @SuppressWarnings("deprecation")
    @Test
    private void testHeadBucket() throws Exception {
        DeleteBucketThread deleteBucket = new DeleteBucketThread( bucketName );
        HeadBucketThread headBucket = new HeadBucketThread( bucketName );
        deleteBucket.start();
        headBucket.start( 10 );
        Assert.assertTrue( deleteBucket.isSuccess(),
                deleteBucket.getErrorMsg() );
        Assert.assertTrue( headBucket.isSuccess(), headBucket.getErrorMsg() );
        Assert.assertFalse( s3Client.doesBucketExist( bucketName ) );

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

    private class HeadBucketThread extends S3ThreadBase {
        String bucketName;

        public HeadBucketThread( String bucketName ) {
            this.bucketName = bucketName;
        }

        @Override
        public void exec() throws Exception {
            AmazonS3 s3Client = CommLib.buildS3Client( acessKeys[ 0 ],
                    acessKeys[ 1 ] );
            try {
                s3Client.headBucket( new HeadBucketRequest( bucketName ) );
            } catch ( AmazonS3Exception e ) {
                Assert.assertEquals( e.getStatusCode(), 404 );
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
}
