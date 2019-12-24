package com.sequoias3.head;

import com.amazonaws.services.s3.AmazonS3;
import com.sequoiadb.exception.BaseException;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.UserUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * test content: doesBucketExist查询桶 testlink-case: seqDB-16661
 *
 * @author wangkexin
 * @Date 2018.12.06
 * @version 1.00
 */

public class TestDoesBucketExist16661 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket16661";
    private String userName = "user16661";
    private String roleName = "normal";
    private AmazonS3 s3Client = null;

    @BeforeClass
    private void setUp() throws Exception {
        CommLib.clearUser( userName );
        String[] accessKeys = UserUtils.createUser( userName, roleName );
        s3Client = CommLib.buildS3Client( accessKeys[ 0 ], accessKeys[ 1 ] );
    }

    @SuppressWarnings("deprecation")
    @Test
    private void testDoesBucketExist() throws Exception {
        s3Client.createBucket( bucketName );
        Assert.assertTrue( s3Client.doesBucketExist( bucketName ) );
        s3Client.deleteBucket( bucketName );
        Assert.assertFalse( s3Client.doesBucketExist( bucketName ) );
        s3Client.createBucket( bucketName );
        Assert.assertTrue( s3Client.doesBucketExist( bucketName ) );
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
}
