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
 * @Description: 不存在桶，查询桶版本控制状态 testlink-case: seqDB-16615
 *
 * @author wangkexin
 * @Date 2018.11.19
 * @version 1.00
 */

public class SetBucketVersioning16615 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket16615";
    private String userName = "user16615";
    private String roleName = "normal";
    private AmazonS3 s3Client = null;

    @BeforeClass
    private void setUp() throws Exception {
        CommLib.clearUser( userName );
        String[] acessKeys = UserUtils.createUser( userName, roleName );
        s3Client = CommLib.buildS3Client( acessKeys[ 0 ], acessKeys[ 1 ] );
        CommLib.clearBucket( s3Client, bucketName );
    }

    @Test
    private void testSwitchBucketVersioning() throws Exception {
        // get bucket versioning status
        try {
            s3Client.getBucketVersioningConfiguration( bucketName ).getStatus();
            Assert.fail( "exp fail but act success" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "NoSuchBucket" );
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
}
