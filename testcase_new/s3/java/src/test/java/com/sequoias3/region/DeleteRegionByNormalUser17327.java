package com.sequoias3.region;

import com.amazonaws.services.s3.model.AmazonS3Exception;
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
 * test content: 非管理员用户删除区域 testlink-case: seqDB-17327
 *
 * @author wangkexin
 * @Date 2019.01.25
 * @version 1.00
 */

public class DeleteRegionByNormalUser17327 extends S3TestBase {
    private String userName = "user17327";
    private String roleName = "normal";
    private String[] accessKeys = null;
    private String regionName = "Beijing17327";
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {
        CommLib.clearUser( userName );
        accessKeys = UserUtils.createUser( userName, roleName );
        CommLib.buildS3Client( accessKeys[ 0 ], accessKeys[ 1 ] );
        RegionUtils.clearRegion( regionName );
    }

    @Test
    public void testCreateRegion() throws Exception {
        Region region = new Region();
        region.withName( regionName );
        RegionUtils.putRegion( region );

        try {
            RegionUtils.deleteRegion( regionName, accessKeys[ 0 ] );
            Assert.fail( "Non-Administrator user delete regions should fail" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "AccessDenied" );
        }

        Assert.assertTrue( RegionUtils.headRegion( regionName ) );
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
