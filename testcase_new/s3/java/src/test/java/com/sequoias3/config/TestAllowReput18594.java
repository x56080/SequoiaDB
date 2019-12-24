package com.sequoias3.config;

import com.amazonaws.services.s3.AmazonS3;
import com.sequoias3.region.Region;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.RegionUtils;
import com.sequoias3.testcommon.s3utils.UserUtils;
import com.sequoias3.user.UserCommDefind;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * test content: 配置允许重复创建桶，创建相同桶 testlink-case: seqDB-18594
 *
 * @author wangkexin
 * @Date 2019.06.25
 * @version 1.00
 */
@Test(groups = "allowreputon") public class TestAllowReput18594
        extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket18594";
    private String userName = "user18594";
    private String[] accessKeys = null;
    private String regionName = "region18594a";
    private String regionName2 = "region18594b";
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
        s3Client.createBucket( bucketName, regionName2 );
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
