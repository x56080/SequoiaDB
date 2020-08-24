package com.sequoias3.region;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoias3.SequoiaS3;
import com.sequoias3.exception.SequoiaS3ServiceException;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.RegionUtils;
import com.sequoias3.testcommon.s3utils.UserUtils;

/**
 * @Description: 非管理员用户删除区域 testlink-case: seqDB-17327
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
    private SequoiaS3 regionClient = null;
    private SequoiaS3 regionClient1 = null;

    @BeforeClass
    private void setUp() throws Exception {
        CommLib.clearUser(userName);
        accessKeys = UserUtils.createUser(userName, roleName);
        CommLib.buildS3Client(accessKeys[0], accessKeys[1]);
        regionClient = CommLib.regionClient();
        RegionUtils.clearRegion(regionClient, regionName);
        regionClient1 = CommLib.regionClient(accessKeys[0], accessKeys[1]);
    }

    @Test
    public void testCreateRegion() throws Exception {
        regionClient.createRegion(regionName);
        try {
            regionClient1.deleteRegion(regionName);
            Assert.fail("Non-Administrator user delete regions should fail");
        } catch (SequoiaS3ServiceException e) {
            Assert.assertEquals(e.getErrorCode(), "AccessDenied");
        }

        Assert.assertTrue(regionClient.headRegion(regionName));
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
