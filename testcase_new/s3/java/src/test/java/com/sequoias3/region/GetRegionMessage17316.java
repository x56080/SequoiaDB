package com.sequoias3.region;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.bean.Region;
import com.sequoias3.testcommon.s3utils.RegionUtils;
import com.sequoias3.testcommon.s3utils.UserUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * test content: 非桶所有者获取桶所在区域信息 testlink-case: seqDB-17316
 *
 * @author wangkexin
 * @Date 2019.01.24
 * @version 1.00
 */

public class GetRegionMessage17316 extends S3TestBase {
    private AmazonS3 s3Client = null;
    private AmazonS3 s3ClientA = null;
    private AmazonS3 s3ClientB = null;
    private String userNameA = "user17316a";
    private String userNameB = "user17316b";
    private String roleName = "normal";
    private String bucketName = "bucket17316";
    private String regionName = "beijing17316";
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {
        CommLib.clearUser( userNameA );
        CommLib.clearUser( userNameB );
        String[] acessKeysA = UserUtils.createUser( userNameA, roleName );
        String[] acessKeysB = UserUtils.createUser( userNameB, roleName );
        s3ClientA = CommLib.buildS3Client( acessKeysA[ 0 ], acessKeysA[ 1 ] );
        s3ClientB = CommLib.buildS3Client( acessKeysB[ 0 ], acessKeysB[ 1 ] );
        s3Client = CommLib.buildS3Client();

        CommLib.clearBucket( s3Client, bucketName );
        RegionUtils.clearRegion( regionName );
    }

    @Test
    public void testGetRegionMessage() throws Exception {
        // create region
        Region region = new Region();
        region.withName( regionName );
        RegionUtils.putRegion( region );

        s3ClientA.createBucket(
                new CreateBucketRequest( bucketName, regionName ) );
        try {
            s3ClientB.getBucketLocation( bucketName );
            Assert.fail( "getBucketLocation by other users should fail!" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "AccessDenied" );
        }

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( runSuccess ) {
                s3ClientA.deleteBucket( bucketName );
                RegionUtils.deleteRegion( regionName );
                UserUtils.deleteUser( userNameA );
                UserUtils.deleteUser( userNameB );
            }
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }
}
