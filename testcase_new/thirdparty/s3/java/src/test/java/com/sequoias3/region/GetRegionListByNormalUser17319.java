package com.sequoias3.region;

import java.util.List;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoias3.SequoiaS3;
import com.sequoias3.model.ListRegionsResult;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.RegionUtils;
import com.sequoias3.testcommon.s3utils.UserUtils;

/**
 * @Description seqDB-17319:非管理员用户获取区域列表信息
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
    private SequoiaS3 regionClient = null;
    private SequoiaS3 regionClient1 = null;

    @BeforeClass
    private void setUp() throws Exception {
        CommLib.clearUser(userName);
        accessKeys = UserUtils.createUser(userName, roleName);
        regionClient = CommLib.regionClient();
        regionClient1 = CommLib.regionClient(accessKeys[0], accessKeys[1]);
        RegionUtils.clearRegion(regionClient, regionName);
    }

    @Test
    public void testCreateRegion() throws Exception {
        regionClient.createRegion(regionName);
        ListRegionsResult listRegionResult = regionClient1.listRegions();
        List<String> regions = listRegionResult.getRegions();
        Assert.assertTrue(regions.contains(regionName.toLowerCase()));
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if (runSuccess) {
                regionClient.deleteRegion(regionName);
                UserUtils.deleteUser(userName);
            }
        } finally {
            regionClient.shutdown();
            regionClient1.shutdown();
        }
    }
}
