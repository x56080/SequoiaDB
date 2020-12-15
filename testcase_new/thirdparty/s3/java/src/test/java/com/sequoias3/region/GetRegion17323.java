package com.sequoias3.region;

import org.json.JSONObject;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoias3.SequoiaS3;
import com.sequoias3.common.DataShardingType;
import com.sequoias3.exception.SequoiaS3ServiceException;
import com.sequoias3.model.CreateRegionRequest;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.RegionUtils;
import com.sequoias3.testcommon.s3utils.UserUtils;
import com.sequoias3.testcommon.s3utils.bean.UserCommDefind;

/**
 * @Description: seqDB-17323 :: 非管理员用户获取区域信息
 * @author fanyu
 * @Date:2019年01月22日
 * @version:1.0
 */
public class GetRegion17323 extends S3TestBase {
    private String regionName = "region17323";
    private String username = "user17323";
    private String accessKeyID = null;
    private boolean runSuccess = false;
    private SequoiaS3 regionClient = null;
    private SequoiaS3 regionClient1 = null;

    @BeforeClass
    private void setUp() throws Exception {
        CommLib.clearUser(username);
        regionClient = CommLib.regionClient();
        RegionUtils.clearRegion(regionClient, regionName);
        JSONObject user = UserUtils.createUser(username, UserCommDefind.normal, UserUtils.accessKeyId);
        accessKeyID = user.getJSONObject(UserCommDefind.accessKeys).getString(UserCommDefind.accessKeyID);
        regionClient1 = CommLib.regionClient(username, accessKeyID);
    }

    @Test
    private void test() throws Exception {
        // create region
        CreateRegionRequest request = new CreateRegionRequest(regionName);
        request.withDataCSShardingType(DataShardingType.MONTH).withDataCLShardingType(DataShardingType.MONTH);
        regionClient.createRegion(request);

        // get region by normal user
        try {
            regionClient1.getRegion(regionName);
            Assert.fail("exp fail but act success," + "regionName = " + regionName + ",username = " + username);
        } catch (SequoiaS3ServiceException e) {
            if (e.getStatusCode() != 403 && !e.getErrorCode().contains("AccessDenied")) {
                throw e;
            }
        }
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if (runSuccess) {
                regionClient.deleteRegion(regionName);
                CommLib.clearUser(username);
            }
        } finally {
            regionClient.shutdown();
            regionClient1.shutdown();
        }

    }
}
