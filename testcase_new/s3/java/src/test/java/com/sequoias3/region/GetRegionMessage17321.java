package com.sequoias3.region;

import java.util.List;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoias3.SequoiaS3;
import com.sequoias3.model.CreateRegionRequest;
import com.sequoias3.model.GetRegionResult;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.RegionUtils;

/**
 * @Description seqDB-17321:获取空区域信息
 * @author wangkexin
 * @Date 2019.01.24
 * @version 1.00
 */

public class GetRegionMessage17321 extends S3TestBase {
    private String regionName = "beijing17321";
    private boolean runSuccess = false;
    private SequoiaS3 regionClient = null;

    @BeforeClass
    private void setUp() throws Exception {
        regionClient = CommLib.regionClient();
        RegionUtils.clearRegion(regionClient, regionName);
    }

    @Test
    public void testGetRegionMessage() throws Exception {
        // create region
        CreateRegionRequest request = new CreateRegionRequest(regionName);
        regionClient.createRegion(request);

        GetRegionResult result = regionClient.getRegion(regionName);
        List<String> bucketList = result.getBuckets();
        Assert.assertEquals(bucketList.size(), 0);

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if (runSuccess) {
                regionClient.deleteRegion(regionName);
            }
        } finally {
            regionClient.shutdown();
        }

    }
}
