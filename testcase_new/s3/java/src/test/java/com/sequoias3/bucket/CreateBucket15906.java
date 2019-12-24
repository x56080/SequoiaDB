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
 * test content: concurrent create same bucket testlink-case: seqDB-15906
 *
 * @author wangkexin
 * @Date 2018.10.16
 * @version 1.00
 */
public class CreateBucket15906 extends S3TestBase {
    private boolean runSuccess = false;
    private String userName = "user15906";
    private String bucketName = "bucket15906";
    private String roleName = "normal";
    private String[] acessKeys = null;
    private AmazonS3 s3Client = null;

    @BeforeClass
    private void setUp() throws Exception {
        CommLib.clearUser( userName );
        acessKeys = UserUtils.createUser( userName, roleName );
        s3Client = CommLib.buildS3Client( acessKeys[ 0 ], acessKeys[ 1 ] );
    }

    @Test
    public void testCreateBucket() throws Exception {
        CreateBucketThread createSameBucket = new CreateBucketThread(
                bucketName );
        createSameBucket.start( 100 );

        Assert.assertTrue( createSameBucket.isSuccess(),
                createSameBucket.getErrorMsg() );

        checkCreateBucketResult( s3Client );
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

    private void checkCreateBucketResult( AmazonS3 s3Client ) {
        // check bucket nums
        List<Bucket> buckets = s3Client.listBuckets();
        Assert.assertEquals( buckets.size(), 1 );

        for ( int i = 0; i < buckets.size(); i++ ) {
            Bucket bucket = buckets.get( i );
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
            } catch ( AmazonS3Exception e ) {
                Assert.assertEquals( e.getErrorCode(),
                        "BucketAlreadyOwnedByYou" );
            } finally {
                if ( s3Client != null ) {
                    s3Client.shutdown();
                }
            }
        }
    }
}
