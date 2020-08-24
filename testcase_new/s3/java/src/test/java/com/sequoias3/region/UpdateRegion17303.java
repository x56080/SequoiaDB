package com.sequoias3.region;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoias3.SequoiaS3;
import com.sequoias3.model.CreateRegionRequest;
import com.sequoias3.model.GetRegionResult;
import com.sequoias3.model.Region;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.RegionUtils;

/**
 * @Description: seqDB-17303 :: 更新区域使用指定模式，指定相同配置
 * @author fanyu
 * @Date:2019年01月22日
 * @version:1.0
 */
public class UpdateRegion17303 extends S3TestBase {
    private String regionName = "region17303";
    private String csName = "cs17303";
    private String[] clNames = { "dataCL17303A", "metaCL17303A", "metaHisCL17303A" };
    private boolean runSuccess = false;
    private SequoiaS3 regionClient = null;

    @BeforeClass
    private void setUp() throws Exception {
        RegionUtils.createCSAndCL(csName, clNames);
        regionClient = CommLib.regionClient();
        RegionUtils.clearRegion(regionClient, regionName);
    }

    @Test
    private void test() throws Exception {
        // create region
        CreateRegionRequest request = new CreateRegionRequest(regionName);
        request.withDataLocation(csName + "." + clNames[0]).withMetaLocation(csName + "." + clNames[1])
                .withMetaHisLocation(csName + "." + clNames[2]);
        regionClient.createRegion(request);
        // put region again
        regionClient.createRegion(request);
        // check region info
        GetRegionResult regionResult = regionClient.getRegion(regionName);
        Region region = request.getRegion();
        checkGetRegionResult(regionResult, region);
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if (runSuccess) {
                RegionUtils.dropCS(new String[] { csName });
                regionClient.deleteRegion(regionName);
            }
        } finally {
            regionClient.shutdown();
        }

    }

    private void checkGetRegionResult(GetRegionResult result, Region expRegion) {
        Region actRegion = result.getRegion();
        Assert.assertEquals(actRegion.getDataLocation(), expRegion.getDataLocation());
        Assert.assertEquals(actRegion.getMetaLocation(), expRegion.getMetaLocation());
        Assert.assertEquals(actRegion.getMetaHisLocation(), expRegion.getMetaHisLocation());
        Assert.assertEquals(result.getBuckets().size(), 0);
    }
}
