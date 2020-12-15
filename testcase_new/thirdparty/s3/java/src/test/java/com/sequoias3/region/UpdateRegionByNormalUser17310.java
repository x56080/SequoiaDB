package com.sequoias3.region;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoias3.SequoiaS3;
import com.sequoias3.common.DataShardingType;
import com.sequoias3.exception.SequoiaS3ServiceException;
import com.sequoias3.model.CreateRegionRequest;
import com.sequoias3.model.GetRegionResult;
import com.sequoias3.model.Region;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.RegionUtils;
import com.sequoias3.testcommon.s3utils.UserUtils;

/**
 * @Description seqDB-17310: 非管理员用户更新区域
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
    private SequoiaS3 regionClient = null;
    private SequoiaS3 regionClient1 = null;

    @BeforeClass
    private void setUp() throws Exception {
        CommLib.clearUser(userName);
        accessKeys = UserUtils.createUser(userName, roleName);
        CommLib.buildS3Client(accessKeys[0], accessKeys[1]);

        regionClient = CommLib.regionClient();
        regionClient1 = CommLib.regionClient(accessKeys[0], accessKeys[1]);
        RegionUtils.clearRegion(regionClient, regionName);

        // create region by administrator
        regionClient.createRegion(regionName);
    }

    @Test
    public void testCreateRegion() throws Exception {
        // update region by normal user
        CreateRegionRequest request = new CreateRegionRequest(regionName);
        request.withDataCSShardingType(DataShardingType.MONTH);
        try {
            regionClient1.createRegion(request);
            Assert.fail("Non-Administrator user update region should fail");
        } catch (SequoiaS3ServiceException e) {
            Assert.assertEquals(e.getErrorCode(), "AccessDenied");
        }

        // check result
        // in shardingType mode,the default value of DataCSShardingType is
        // 'year',the default value of DataCLShardingType is 'quarter'.
        GetRegionResult result = regionClient.getRegion(regionName);
        Region currRegion = result.getRegion();
        Assert.assertEquals(currRegion.getName(), regionName.toLowerCase());
        Assert.assertEquals(currRegion.getDataCSShardingType(), DataShardingType.YEAR);
        Assert.assertEquals(currRegion.getDataCLShardingType(), DataShardingType.QUARTER);

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
            regionClient1.shutdown();
            regionClient.shutdown();
        }

    }
}
