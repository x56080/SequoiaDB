package com.sequoias3.region;

import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.RegionUtils;
import com.sequoias3.testcommon.s3utils.UserUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * test content: 非管理员用户更新区域 testlink-case: seqDB-17310
 *
 * @author wangkexin
 * @Date 2019.01.23
 * @version 1.00
 */

public class UpdateRegionByNormalUser17310 extends S3TestBase {
    private String userName = "user17310";
    private String roleName = "normal";
    private String[] accessKeys = null;
    private String regionName = "Beijing17310";
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {
        CommLib.clearUser( userName );
        accessKeys = UserUtils.createUser( userName, roleName );
        CommLib.buildS3Client( accessKeys[ 0 ], accessKeys[ 1 ] );

        RegionUtils.clearRegion( regionName );

        // create region by administrator
        Region region = new Region();
        region.withName( regionName );
        RegionUtils.putRegion( region );
    }

    @Test
    public void testCreateRegion() throws Exception {
        // create same region by normal user
        Region sameRegion = new Region();
        sameRegion.withName( regionName );
        try {
            RegionUtils.putRegion( sameRegion, accessKeys[ 0 ] );
            Assert.fail( "Non-Administrator user put region should fail" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "AccessDenied" );
        }

        // update region by normal user
        Region newRegion = new Region();
        newRegion.withName( regionName ).withDataCSShardingType( "month" );
        try {
            RegionUtils.putRegion( newRegion, accessKeys[ 0 ] );
            Assert.fail( "Non-Administrator user update region should fail" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "AccessDenied" );
        }

        // check result
        // in shardingType mode,the default value of DataCSShardingType is
        // 'year',the default value of DataCLShardingType is 'quarter'.
        GetRegionResult result = RegionUtils.getRegion( regionName );
        Region currRegion = result.getRegion();
        Assert.assertEquals( currRegion.getName(), regionName.toLowerCase() );
        Assert.assertEquals( currRegion.getDataCSShardingType(), "year" );
        Assert.assertEquals( currRegion.getDataCLShardingType(), "quarter" );

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        if ( runSuccess ) {
            RegionUtils.deleteRegion( regionName );
            UserUtils.deleteUser( userName );
        }
    }
}
