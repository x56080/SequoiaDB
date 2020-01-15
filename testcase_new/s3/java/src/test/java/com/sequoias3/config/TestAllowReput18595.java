package com.sequoias3.config;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.bean.Region;
import com.sequoias3.testcommon.s3utils.RegionUtils;
import com.sequoias3.testcommon.s3utils.UserUtils;
import com.sequoias3.testcommon.s3utils.bean.UserCommDefind;

/**
 * test content: 配置不允许重复创建桶，创建相同桶 testlink-case: seqDB-18595
 *
 * @author wangkexin
 * @Date 2019.06.25
 * @version 1.00
 */
public class TestAllowReput18595 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket18595";
    private String userName = "user18595";
    private String[] accessKeys = null;
    private String regionName = "region18595a";
    private String regionName2 = "region18595b";
    private AmazonS3 s3Client = null;

    @BeforeClass
    private void setUp() throws Exception {
        CommLib.clearUser( userName );
        RegionUtils.clearRegion( regionName );
        RegionUtils.clearRegion( regionName2 );
        accessKeys = UserUtils.createUser( userName, UserCommDefind.normal );
        s3Client = CommLib.buildS3Client( accessKeys[ 0 ], accessKeys[ 1 ] );

        Region region = new Region();
        region.withName( regionName );
        RegionUtils.putRegion( region );

        Region region2 = new Region();
        region2.withName( regionName2 );
        RegionUtils.putRegion( region2 );
    }

    @SuppressWarnings("deprecation")
    @Test
    private void testReputBacket() {
        s3Client.createBucket( bucketName, regionName );
        Assert.assertTrue( s3Client.doesBucketExist( bucketName ) );

        // reput same bucket
        try {
            s3Client.createBucket( bucketName, regionName2 );
            Assert.fail(
                    "set allowreput config off, than reput bucket by the same user should be failed." );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "BucketAlreadyOwnedByYou" );
        }
        Assert.assertEquals( s3Client.listBuckets().size(), 1 );
        Assert.assertTrue( s3Client.doesBucketExist( bucketName ) );
        Assert.assertEquals( s3Client.getBucketLocation( bucketName ),
                regionName );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( runSuccess ) {
                CommLib.clearUser( userName );
                RegionUtils.deleteRegion( regionName );
                RegionUtils.deleteRegion( regionName2 );
            }
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }
}
