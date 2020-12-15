package com.sequoias3.region;

import java.util.concurrent.atomic.AtomicInteger;

import org.springframework.web.client.HttpServerErrorException;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import com.sequoiadb.base.Sequoiadb;
import com.sequoias3.SequoiaS3;
import com.sequoias3.common.DataShardingType;
import com.sequoias3.exception.SequoiaS3ServiceException;
import com.sequoias3.model.CreateRegionRequest;
import com.sequoias3.model.GetRegionResult;
import com.sequoias3.model.Region;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import com.sequoias3.testcommon.s3utils.RegionUtils;

/**
 * @Description seqDB-17352: PutRegion接口参数校验
 * @author wangkexin
 * @Date 2019.01.28
 * @version 1.00
 */
public class TestPutRegion17352 extends S3TestBase {
    private String dataDomain = "dataDomain17352";
    private String metaDomain = "metaDomain17352";
    private String metaCSName = "metaCS17352";
    private String dataCSName = "dataCS17352";
    private String[] metaClNames = { "metaCL17352", "metaHistoryCL17352" };
    private String[] dataClName = { "dataCL17352" };
    private AtomicInteger actSuccessTests = new AtomicInteger(0);
    private SequoiaS3 regionClient = null;

    @DataProvider(name = "legalRegionNameProvider")
    public Object[][] generateRegionName() {
        return new Object[][] {
                // test a : 范围内取值
                new Object[] { "beijing-1", "specifiedMode", null, null },
                new Object[] { "shanghai", "specifiedMode", null, null },
                // test b : 长度边界值
                new Object[] { ObjectUtils.getRandomString(3), "specifiedMode", null, null },
                new Object[] { ObjectUtils.getRandomString(20), "shardingTypeMode", DataShardingType.YEAR,
                        DataShardingType.QUARTER },
                // test c : 包含字母 数字字符[0-9a-zA-Z]
                new Object[] { "01abcdefgABCDEFG", "shardingTypeMode", DataShardingType.QUARTER,
                        DataShardingType.MONTH },
                new Object[] { "234hijklmnHIJKLMN", "shardingTypeMode", DataShardingType.MONTH, DataShardingType.YEAR },
                new Object[] { "567opqrstuOPQRSTU", "specifiedMode", null, null },
                new Object[] { "89vwxyzVWXYZ", "specifiedMode", null, null }, };
    }

    @DataProvider(name = "illegalRegionNameProvider")
    public Object[][] generateIllegalRegionName() {
        return new Object[][] {
                // test a : 超过边界值
                new Object[] { "" }, new Object[] { ObjectUtils.getRandomString(2) },
                new Object[] { ObjectUtils.getRandomString(21) },
                // test b : 包含特殊字符
                new Object[] { "test!" }, new Object[] { "test_" }, new Object[] { "test." }, new Object[] { "test*" },
                new Object[] { "test'" }, new Object[] { "test(" }, new Object[] { "test)" }, new Object[] { "中文" }, };
    }

    @BeforeClass
    private void setUp() throws Exception {
        RegionUtils.dropDomain(metaDomain);
        RegionUtils.dropDomain(dataDomain);

        RegionUtils.createCSAndCL(metaCSName, metaClNames);
        RegionUtils.createCSAndCL(dataCSName, dataClName);

        RegionUtils.createDomain(dataDomain);
        RegionUtils.createDomain(metaDomain);
        regionClient = CommLib.regionClient();
    }

    @Test(dataProvider = "legalRegionNameProvider")
    public void legalRegionName(String regionName, String mode, DataShardingType dataCSShardingType,
            DataShardingType dataCLShardingType) throws Exception {

        RegionUtils.clearRegion(regionClient, regionName);
        // test a : specified mode
        CreateRegionRequest request = new CreateRegionRequest(regionName);
        if (mode.equals("specifiedMode")) {
            request.withMetaLocation(metaCSName + "." + metaClNames[0])
                    .withMetaHisLocation(metaCSName + "." + metaClNames[1])
                    .withDataLocation(dataCSName + "." + dataClName[0]);
            regionClient.createRegion(request);
            checkSpecifiedMode(regionName);
        }

        if (mode.equals("shardingTypeMode")) {
            request.withDataCSShardingType(dataCSShardingType).withDataCLShardingType(dataCLShardingType)
                    .withDataDomain(dataDomain).withMetaDomain(metaDomain);
            regionClient.createRegion(request);

            checkShardingTypeMode(regionName, dataCSShardingType, dataCLShardingType);
        }
        regionClient.deleteRegion(regionName);
        actSuccessTests.getAndIncrement();
    }

    @Test(dataProvider = "illegalRegionNameProvider")
    public void illegalRegionName(String regionName) throws Exception {
        try {
            regionClient.createRegion(regionName);
            Assert.fail("put region with illegal region name should fail!");
        } catch (IllegalArgumentException e) {
            Assert.assertTrue(e.getMessage().contains("region name is invalid"), e.getMessage());
        }
        actSuccessTests.getAndIncrement();
    }

    @Test
    public void illegalParameterName() throws Exception {
        String regionName = "region17352";
        // specified mode
        CreateRegionRequest request = new CreateRegionRequest(regionName);
        try {
            request.withMetaLocation("").withMetaHisLocation(metaCSName + "." + metaClNames[1])
                    .withDataLocation(dataCSName + "." + dataClName[0]);
            regionClient.createRegion(request);
            Assert.fail("put region with illegal metaLocation should fail!");
        } catch (SequoiaS3ServiceException e) {
            Assert.assertEquals(e.getErrorCode(), "InvalidLocation");
        }
        CreateRegionRequest request1 = new CreateRegionRequest(regionName);
        try {
            request1.withMetaLocation(metaCSName + "." + metaClNames[0]).withMetaHisLocation("")
                    .withDataLocation(dataCSName + "." + dataClName[0]);
            regionClient.createRegion(request1);
            Assert.fail("put region with illegal metaHisLocation should fail!");
        } catch (SequoiaS3ServiceException e) {
            Assert.assertEquals(e.getErrorCode(), "InvalidLocation");
        }

        CreateRegionRequest request2 = new CreateRegionRequest(regionName);
        try {
            request2.withMetaLocation(metaCSName + "." + metaClNames[0])
                    .withMetaHisLocation(metaCSName + "." + metaClNames[1]).withDataLocation("");
            regionClient.createRegion(request2);
            Assert.fail("put region with illegal dataLocation should fail!");
        } catch (SequoiaS3ServiceException e) {
            Assert.assertEquals(e.getErrorCode(), "InvalidLocation");
        }

        // DataLobPageSize DataReplSize
        int[] dataLobPageSizes = { -1, 65535, 524289 };
        int[] dataReplSizes = { -2, 8 };
        CreateRegionRequest request3 = new CreateRegionRequest(regionName);
        for (int dataLobPageSize : dataLobPageSizes) {
            try {
                request3.withDataCSShardingType(DataShardingType.YEAR).withDataCLShardingType(DataShardingType.MONTH)
                        .withDataLobPageSize(dataLobPageSize);
                regionClient.createRegion(request3);
                Assert.fail("put region with illegal dataLobPageSizes should fail!");
            } catch (SequoiaS3ServiceException e) {
                Assert.assertEquals(e.getErrorCode(), "InvalidLobPageSize");
            } catch (HttpServerErrorException e) {
                Assert.assertEquals(e.getStatusCode().value(), 500);
            }
        }

        CreateRegionRequest request4 = new CreateRegionRequest(regionName);
        for (int dataReplSize : dataReplSizes) {
            try {
                request4.withDataReplSize(dataReplSize);
                regionClient.createRegion(request4);
                Assert.fail("put region with illegal dataReplSize should fail!");
            } catch (SequoiaS3ServiceException e) {
                Assert.assertEquals(e.getErrorCode(), "InvalidReplSize");
            } catch (HttpServerErrorException e) {
                Assert.assertEquals(e.getStatusCode().value(), 500);
            }
        }
        actSuccessTests.getAndIncrement();
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if (actSuccessTests.get() == (generateRegionName().length + generateIllegalRegionName().length + 1)) {
                try (Sequoiadb sdb = new Sequoiadb(S3TestBase.coordUrl, "", "")) {
                    sdb.dropCollectionSpace(metaCSName);
                    sdb.dropCollectionSpace(dataCSName);
                    sdb.dropDomain(dataDomain);
                    sdb.dropDomain(metaDomain);
                }
            }
        } finally {
            regionClient.shutdown();
        }

    }

    private void checkSpecifiedMode(String regionName) throws Exception {
        Assert.assertTrue(regionClient.headRegion(regionName));
        GetRegionResult result = regionClient.getRegion(regionName);
        Region region = result.getRegion();
        Assert.assertEquals(region.getName(), regionName.toLowerCase());
        Assert.assertEquals(region.getMetaLocation(), metaCSName + "." + metaClNames[0]);
        Assert.assertEquals(region.getMetaHisLocation(), metaCSName + "." + metaClNames[1]);
        Assert.assertEquals(region.getDataLocation(), dataCSName + "." + dataClName[0]);
    }

    private void checkShardingTypeMode(String regionName, DataShardingType dataCSShardingType,
            DataShardingType dataCLShardingType) throws Exception {
        Assert.assertTrue(regionClient.headRegion(regionName));
        GetRegionResult result = regionClient.getRegion(regionName);
        Region region = result.getRegion();
        Assert.assertEquals(region.getName(), regionName.toLowerCase());
        Assert.assertEquals(region.getDataCSShardingType(), dataCSShardingType);
        Assert.assertEquals(region.getDataCLShardingType(), dataCLShardingType);
        Assert.assertEquals(region.getMetaDomain(), metaDomain);
        Assert.assertEquals(region.getDataDomain(), dataDomain);
    }
}
