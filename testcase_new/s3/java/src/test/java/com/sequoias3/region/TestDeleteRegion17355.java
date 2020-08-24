package com.sequoias3.region;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.Sequoiadb;
import com.sequoias3.SequoiaS3;
import com.sequoias3.exception.SequoiaS3ServiceException;
import com.sequoias3.model.CreateRegionRequest;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.RegionUtils;

/**
 * @Description seqDB-17355:DeleteRegion接口参数校验
 * @author wangkexin
 * @Date 2019.01.24
 * @version 1.00
 */

public class TestDeleteRegion17355 extends S3TestBase {
    private String regionName = "Beijing17355";
    private String metaCSName = "metaCS17355";
    private String dataCSName = "dataCS17355";
    private String[] metaClNames = { "metaCL17355", "metaHistoryCL17355" };
    private String[] dataClName = { "dataCL17355" };
    private SequoiaS3 regionClient = null;

    @BeforeClass
    private void setUp() throws Exception {
        RegionUtils.createCSAndCL(metaCSName, metaClNames);
        RegionUtils.createCSAndCL(dataCSName, dataClName);

        regionClient = CommLib.regionClient();
        RegionUtils.clearRegion(regionClient, regionName);

        CreateRegionRequest request = new CreateRegionRequest(regionName);
        request.withMetaLocation(metaCSName + "." + metaClNames[0])
                .withMetaHisLocation(metaCSName + "." + metaClNames[1])
                .withDataLocation(dataCSName + "." + dataClName[0]);
        regionClient.createRegion(request);
    }

    @Test
    public void testCreateRegion() throws Exception {
        // 合法值
        regionClient.deleteRegion(regionName);
        Assert.assertFalse(regionClient.headRegion(regionName));

        // 非法值 SEQUOIADBMAINSTREAM-4186
        try {
            regionClient.deleteRegion("");
            Assert.fail("put region with illegal region name '' should fail!");
        } catch (SequoiaS3ServiceException e) {
            Assert.assertEquals(e.getErrorCode(), "InvalidRegionName");
        }

        try {
            regionClient.deleteRegion(new String());
            Assert.fail("put region with illegal region name null should fail!");
        } catch (SequoiaS3ServiceException e) {
            Assert.assertEquals(e.getErrorCode(), "InvalidRegionName");
        }
    }

    @AfterClass
    private void tearDown() throws Exception {
        try (Sequoiadb sdb = new Sequoiadb(S3TestBase.coordUrl, "", "")) {
            sdb.dropCollectionSpace(dataCSName);
            sdb.dropCollectionSpace(metaCSName);
        } finally {
            regionClient.shutdown();
        }
    }
}
