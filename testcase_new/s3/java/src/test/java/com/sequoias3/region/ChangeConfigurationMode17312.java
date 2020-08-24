package com.sequoias3.region;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.Sequoiadb;
import com.sequoias3.SequoiaS3;
import com.sequoias3.common.DataShardingType;
import com.sequoias3.exception.SequoiaS3ServiceException;
import com.sequoias3.model.CreateRegionRequest;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.RegionUtils;

/**
 * @Description seqDB-17312:创建和更新区域配置方式不同
 * @author wangkexin
 * @Date 2019.01.23
 * @version 1.00
 */

public class ChangeConfigurationMode17312 extends S3TestBase {
    private String specifiedRegionName = "ModeOne17312";
    private String shardingTypeRegionName = "ModeTwo17312";
    private String metaCSName = "metaCS17312";
    private String dataCSName = "dataCS17312";
    private String[] metaClNames = { "metaCL17312", "metaHistoryCL17312" };
    private String[] dataClName = { "dataCL17312" };
    private boolean runSuccess = false;
    private SequoiaS3 regionClient = null;

    @BeforeClass
    private void setUp() throws Exception {
        RegionUtils.createCSAndCL(metaCSName, metaClNames);
        RegionUtils.createCSAndCL(dataCSName, dataClName);
        regionClient = CommLib.regionClient();
        RegionUtils.clearRegion(regionClient, specifiedRegionName);
        RegionUtils.clearRegion(regionClient, shardingTypeRegionName);
    }

    @Test
    public void testCreateRegion() throws Exception {
        // test a : change the specified mode to ShardingType mode
        CreateRegionRequest request = new CreateRegionRequest(specifiedRegionName);
        request.withMetaLocation(metaCSName + "." + metaClNames[0])
                .withMetaHisLocation(metaCSName + "." + metaClNames[1])
                .withDataLocation(dataCSName + "." + dataClName[0]);
        regionClient.createRegion(request);

        Assert.assertTrue(regionClient.headRegion(specifiedRegionName));

        request.withDataCSShardingType(DataShardingType.YEAR).withDataCLShardingType(DataShardingType.MONTH);

        try {
            regionClient.createRegion(request);
            Assert.fail("change the specified mode to ShardingType mode should fail");
        } catch (SequoiaS3ServiceException e) {
            Assert.assertEquals(e.getErrorCode(), "ConflictRegionType");
            Assert.assertEquals(e.getStatusCode(), 409);
        }

        // test b : change ShardingType mode to the specified mode
        CreateRegionRequest requestb = new CreateRegionRequest(shardingTypeRegionName);
        regionClient.createRegion(requestb);
        Assert.assertTrue(regionClient.headRegion(shardingTypeRegionName));
        requestb.withMetaLocation(metaCSName + "." + metaClNames[0])
                .withMetaHisLocation(metaCSName + "." + metaClNames[1])
                .withDataLocation(dataCSName + "." + dataClName[0]);

        try {
            regionClient.createRegion(requestb);
            Assert.fail("change ShardingType mode to the specified mode should fail");
        } catch (SequoiaS3ServiceException e) {
            Assert.assertEquals(e.getErrorCode(), "ConflictRegionType");
            Assert.assertEquals(e.getStatusCode(), 409);
        }
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        if (runSuccess) {
            try (Sequoiadb sdb = new Sequoiadb(S3TestBase.coordUrl, "", "");) {
                sdb.dropCollectionSpace(metaCSName);
                sdb.dropCollectionSpace(dataCSName);
                regionClient.deleteRegion(specifiedRegionName);
                regionClient.deleteRegion(shardingTypeRegionName);
            } finally {
                regionClient.shutdown();
            }
        }
    }
}
