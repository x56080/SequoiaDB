package com.sequoias3.region;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.Sequoiadb;
import com.sequoias3.SequoiaS3;
import com.sequoias3.model.CreateRegionRequest;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.RegionUtils;

/**
 * @Description seqDB-17356:HeadRegion接口参数校验
 * @author wangkexin
 * @Date 2019.01.24
 * @version 1.00
 */

public class TestHeadRegion17356 extends S3TestBase {
    private String regionName = "Beijing17356";
    private String metaCSName = "metaCS17356";
    private String dataCSName = "dataCS17356";
    private String[] metaClNames = { "metaCL17356", "metaHistoryCL17356" };
    private String[] dataClName = { "dataCL17356" };
    private boolean runSuccess = false;
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
        Assert.assertTrue(regionClient.headRegion(regionName));
        // 非法值
        Boolean flag1 = regionClient.headRegion("");
        Assert.assertFalse(flag1, " headRegin region with '' fail,return false(404) ");

        Boolean flag2 = regionClient.headRegion(new String());
        Assert.assertFalse(flag2, " headRegin region with null fail,return false(404) ");

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        if (runSuccess) {
            try (Sequoiadb sdb = new Sequoiadb(S3TestBase.coordUrl, "", "")) {
                regionClient.deleteRegion(regionName);
                sdb.dropCollectionSpace(dataCSName);
                sdb.dropCollectionSpace(metaCSName);
            } finally {
                regionClient.shutdown();
            }
        }
    }
}
