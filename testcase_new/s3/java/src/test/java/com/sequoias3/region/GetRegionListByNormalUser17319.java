package com.sequoias3.region;

import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.bean.Region;
import com.sequoias3.testcommon.s3utils.RegionUtils;
import com.sequoias3.testcommon.s3utils.UserUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.List;

/**
 * test content: 非管理员用户获取区域列表信息 testlink-case: seqDB-17319
 *
 * @author wangkexin
 * @Date 2019.01.24
 * @version 1.00
 */

public class GetRegionListByNormalUser17319 extends S3TestBase {
    private String userName = "user17319";
    private String roleName = "normal";
    private String[] accessKeys = null;
    private String regionName = "Beijing17319";
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

        List<String> regions = RegionUtils.listRegions( accessKeys[ 0 ] );
        Assert.assertTrue( regions.contains( regionName.toLowerCase() ) );
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
