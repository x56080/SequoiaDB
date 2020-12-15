package com.sequoias3.config;

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

/**
 * @Description seqDB-18612: 更新区域配置lobPageSize和replSize
 * @author wangkexin
 * @Date 2019.06.27
 * @version 1.00
 */
public class UpdateRegion18612 extends S3TestBase {
    private boolean runSuccess = false;
    private String regionName = "region18612";
    private SequoiaS3 regionClient = null;

    @BeforeClass
    private void setUp() throws Exception {
        regionClient = CommLib.regionClient();
        RegionUtils.clearRegion(regionClient, regionName);
    }

    @Test
    private void testUpdateRegion() throws Exception {
        // create region
        int dataLobPageSize = 8192;
        int dataReplSize = 1;
        CreateRegionRequest request = new CreateRegionRequest(regionName);
        request.withDataCSShardingType(DataShardingType.YEAR).withDataCLShardingType(DataShardingType.YEAR)
                .withDataLobPageSize(dataLobPageSize).withDataReplSize(dataReplSize);
        regionClient.createRegion(request);
        // test a:.只更新配置lobPageSize（修改为其他合法值）
        CreateRegionRequest requesta = new CreateRegionRequest(regionName);
        requesta.withDataLobPageSize(4096);
        try {
            // update region
            regionClient.createRegion(requesta);
            Assert.fail("exp failed but act success,region = " + requesta.toString());
        } catch (SequoiaS3ServiceException e) {
            Assert.assertEquals(e.getErrorCode(), "ConflictLobPageSize");
        }
        checkLobPageSizeAndReplSize(dataLobPageSize, dataReplSize);

        // test b:.只更新配置replSize（修改为其他合法值）
        CreateRegionRequest requestb = new CreateRegionRequest(regionName);
        requestb.withDataReplSize(2);
        try {
            // update region
            regionClient.createRegion(requestb);
            Assert.fail("exp failed but act success,region = " + requestb.toString());
        } catch (SequoiaS3ServiceException e) {
            Assert.assertEquals(e.getErrorCode(), "ConflictReplSize");
        }
        checkLobPageSizeAndReplSize(dataLobPageSize, dataReplSize);

        // test c:同时更新配置lobPageSize和replSize
        CreateRegionRequest requestc = new CreateRegionRequest(regionName);
        requestc.withDataLobPageSize(16384).withDataReplSize(3);
        try {
            // update region
            regionClient.createRegion(requestc);
            Assert.fail("exp failed but act success,region = " + requestc.toString());
        } catch (SequoiaS3ServiceException e) {
            Assert.assertEquals(e.getErrorCode(), "ConflictLobPageSize");
        }
        checkLobPageSizeAndReplSize(dataLobPageSize, dataReplSize);
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

    private void checkLobPageSizeAndReplSize(int dataLobPageSize, int dataReplSize) throws Exception {
        GetRegionResult result = regionClient.getRegion(regionName);
        Region region = result.getRegion();
        Assert.assertEquals(region.getDataLobPageSize().intValue(), dataLobPageSize);
        Assert.assertEquals(region.getDataReplSize().intValue(), dataReplSize);
    }
}
