package com.sequoias3.bucket;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.sequoiadb.exception.BaseException;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.S3ThreadBase;
import com.sequoias3.testcommon.s3utils.UserUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.ArrayList;
import java.util.List;

/**
 * test content: concurrent delete bucket and sequoiadb abnormal testlink-case:
 * seqDB-15919
 *
 * @author wangkexin
 * @Date 2018.10.17
 * @version 1.00
 */
public class CreateBucket15919 extends S3TestBase {
    private final int defaultNums = 100;
    private boolean runSuccess = false;
    private String userName = "user15919";
    private String bucketName = "bucket15919";
    private String roleName = "normal";
    private AmazonS3 s3Client = null;
    private String[] acessKeys = null;

    @BeforeClass
    private void setUp() throws Exception {
        CommLib.clearUser( userName );
        acessKeys = UserUtils.createUser( userName, roleName );
        s3Client = CommLib.buildS3Client( acessKeys[ 0 ], acessKeys[ 1 ] );
        createBuckets();
    }

    @Test
    private void testDeleteBucket() throws Exception {
        List<DeleteBucketThread> deleteBuckets = new ArrayList<>( 20 );
        for ( int i = 0; i < defaultNums; i++ ) {
            String subBucketName = bucketName + "." + i;
            deleteBuckets.add( new DeleteBucketThread( subBucketName ) );
        }

        for ( DeleteBucketThread deleteBucket : deleteBuckets ) {
            deleteBucket.start();
        }

        for ( DeleteBucketThread deleteBucket : deleteBuckets ) {
            Assert.assertTrue( deleteBucket.isSuccess(),
                    deleteBucket.getErrorMsg() );
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

    private void createBuckets() {
        for ( int i = 0; i < defaultNums; i++ ) {
            String subBucketName = bucketName + "." + i;
            s3Client.createBucket( subBucketName );
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
