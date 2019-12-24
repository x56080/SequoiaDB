package com.sequoias3.bucket;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.UserUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * test content: 不同用户设置桶版本控制状态 testlink-case: seqDB-16614
 *
 * @author wangkexin
 * @Date 2018.11.19
 * @version 1.00
 */

public class SetBucketVersioning16614 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket16614";
    private String userName_a = "user16614a";
    private String userName_b = "user16614b";
    private String roleName = "normal";
    private AmazonS3 s3ClientA = null;
    private AmazonS3 s3ClientB = null;

    @BeforeClass
    private void setUp() throws Exception {
        CommLib.clearUser( userName_a );
        CommLib.clearUser( userName_b );
        String[] acessKeysA = UserUtils.createUser( userName_a, roleName );
        s3ClientA = CommLib.buildS3Client( acessKeysA[ 0 ], acessKeysA[ 1 ] );

        String[] acessKeysB = UserUtils.createUser( userName_b, roleName );
        s3ClientB = CommLib.buildS3Client( acessKeysB[ 0 ], acessKeysB[ 1 ] );

        // user A create bucket
        s3ClientA.createBucket( bucketName );
    }

    @Test
    private void testSwitchBucketVersioning() throws Exception {
        // set bucket versioning status by user B
        try {
            CommLib.setBucketVersioning( s3ClientB, bucketName, "Enabled" );
            Assert.fail( "exp fail but act success" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "AccessDenied" );
        }

        CommLib.setBucketVersioning( s3ClientA, bucketName, "Enabled" );
        Assert.assertEquals(
                s3ClientA.getBucketVersioningConfiguration( bucketName )
                        .getStatus(), "Enabled" );

        try {
            CommLib.setBucketVersioning( s3ClientB, bucketName, "Suspended" );
            Assert.fail( "exp fail but act success" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "AccessDenied" );
        }
        Assert.assertEquals(
                s3ClientA.getBucketVersioningConfiguration( bucketName )
                        .getStatus(), "Enabled" );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( runSuccess ) {
                UserUtils.deleteUser( userName_a );
                UserUtils.deleteUser( userName_b );
            }
        } finally {
            if ( s3ClientA != null ) {
                s3ClientA.shutdown();
            }
            if ( s3ClientB != null ) {
                s3ClientB.shutdown();
            }
        }
    }
}
