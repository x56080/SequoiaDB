package com.sequoias3.bucket;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.Bucket;
import com.amazonaws.services.s3.model.Owner;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.UserUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.List;

/**
 * test content: 桶版本控制状态参数校验 testlink-case: seqDB-16611
 *
 * @author wangkexin
 * @Date 2018.11.19
 * @version 1.00
 */

public class SetBucketVersioning16611 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket16611";
    private String userName = "user16611";
    private String roleName = "normal";
    private AmazonS3 s3Client = null;

    @BeforeClass
    private void setUp() throws Exception {
        CommLib.clearUser( userName );
        String[] acessKeys = UserUtils.createUser( userName, roleName );
        s3Client = CommLib.buildS3Client( acessKeys[ 0 ], acessKeys[ 1 ] );
    }

    @Test
    private void testSetBucketVersioning() throws Exception {
        // create bucket
        s3Client.createBucket( bucketName );
        checkCreateBucketResult();
        // check bucket versioning status
        Assert.assertEquals( s3Client
                .getBucketVersioningConfiguration( bucketName ).getStatus(),
                "Off" );

        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );
        Assert.assertEquals( s3Client
                .getBucketVersioningConfiguration( bucketName ).getStatus(),
                "Enabled" );

        try {
            CommLib.setBucketVersioning( s3Client, bucketName, "null" );
            Assert.fail( "exp fail but found success" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "InvalidVersioningStatus" );
        }

        CommLib.setBucketVersioning( s3Client, bucketName, "Suspended" );
        Assert.assertEquals( s3Client
                .getBucketVersioningConfiguration( bucketName ).getStatus(),
                "Suspended" );

        try {
            CommLib.setBucketVersioning( s3Client, bucketName, "abc" );
            Assert.fail( "exp fail but found success" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "InvalidVersioningStatus" );
        }
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

    private void checkCreateBucketResult() {
        // create one bucket,check the bucket name and owner name
        List< Bucket > buckets = s3Client.listBuckets();
        boolean findBucketFlag = false;
        for ( int i = 0; i < buckets.size(); i++ ) {
            String actBucketName = buckets.get( i ).getName();
            // get the create bucket,then check the bucket name and owner
            if ( actBucketName.equals( bucketName ) ) {
                Owner actOwner = buckets.get( i ).getOwner();
                Assert.assertEquals( actOwner.getDisplayName(), userName );
                findBucketFlag = true;
                break;
            }
        }
        Assert.assertTrue( findBucketFlag, " the bucket must be exist!" );
    }
}
