package com.sequoias3.bucket;

import com.amazonaws.services.s3.AmazonS3;
import com.sequoiadb.exception.BaseException;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.S3ThreadBase;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.ArrayList;
import java.util.List;

/**
 * @Description seqDB-15907:concurrent create bucket and sequoiadb abnormal
 * @author wuyan
 * @Date 2018.10.10
 * @version 1.00
 */
public class CreateBucket15907 extends S3TestBase {
    // private String clientRegion = "us-east-1";
    private final int defaultNums = 100;
    private boolean runSuccess = false;
    private String bucketName = "bucket15907aaa";
    private AmazonS3 s3Client = null;

    @BeforeClass(enabled = false)
    private void setUp() throws Exception {
        s3Client = CommLib.buildS3Client();
        CommLib.clearBuckets( s3Client );
    }

    @Test(enabled = false)
    public void testCreateBucket() throws Exception {
        List<CreateBucketThread> createBuckets = new ArrayList<>( 20 );

        for ( int i = 0; i < defaultNums; i++ ) {
            String subBucketName = bucketName + "." + i;
            createBuckets.add( new CreateBucketThread( subBucketName ) );
        }

        for ( CreateBucketThread createBucket : createBuckets ) {
            createBucket.start();
        }

        for ( CreateBucketThread createBucket : createBuckets ) {
            Assert.assertTrue( createBucket.isSuccess(),
                    createBucket.getErrorMsg() );
        }

        // checkDeleteBucketResult(s3Client, existBuckets);
        runSuccess = true;
    }

    @AfterClass(enabled = false)
    private void tearDown() throws Exception {
        try {
            if ( runSuccess ) {
                s3Client.deleteBucket( bucketName );
                CommLib.clearBuckets( s3Client );
            }
        } catch ( BaseException e ) {
            Assert.fail( "clean up failed:" + e.getMessage() );
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }

        }
    }

    private class CreateBucketThread extends S3ThreadBase {
        String bucketName;

        public CreateBucketThread( String bucketName ) {
            this.bucketName = bucketName;
        }

        @Override
        public void exec() throws Exception {
            AmazonS3 s3Client = CommLib.buildS3Client();
            try {
                System.out.println( "$$$$$$$---begin to create:" + bucketName );
                s3Client.createBucket( bucketName );
                System.out.println( "$$$$$$$---end to create:" + bucketName );
            } finally {
                if ( s3Client != null ) {
                    s3Client.shutdown();
                }
            }
        }
    }

}
