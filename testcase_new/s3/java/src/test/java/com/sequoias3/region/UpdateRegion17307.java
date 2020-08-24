package com.sequoias3.region;

import java.util.concurrent.atomic.AtomicInteger;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import com.sequoias3.SequoiaS3;
import com.sequoias3.common.DataShardingType;
import com.sequoias3.exception.SequoiaS3ServiceException;
import com.sequoias3.model.CreateRegionRequest;
import com.sequoias3.model.GetRegionResult;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.RegionUtils;

/**
 * @Description: seqDB-17307 :: 更新区域配置domain，指定domain不存在
 * @author fanyu
 * @Date:2019年01月22日
 * @version:1.0
 */
public class UpdateRegion17307 extends S3TestBase {
    private String[] regionNames = new String[] { "region17307a", "region17307b" };
    private String existDomain = "domain17307";
    private String updateDomain = "doesNotExistdomain17307";
    private AtomicInteger actSuccessTests = new AtomicInteger(0);
    private SequoiaS3 regionClient = null;

    @BeforeClass
    private void setUp() throws Exception {
        for (String regionName : regionNames) {
            regionClient = CommLib.regionClient();
            RegionUtils.clearRegion(regionClient, regionName);
        }
    }

    @DataProvider(name = "range-provider")
    private Object[][] rangeData() {
        RegionUtils.createDomain(existDomain);
        return new Object[][] {
                // regionName dataDomain metaDomain upDataDomain upMeatDomain
                { regionNames[0], existDomain, existDomain, updateDomain, existDomain },
                { regionNames[1], existDomain, existDomain, existDomain, updateDomain }, };
    }

    @Test(dataProvider = "range-provider")
    private void test(String regionName, String dataDomain, String metaDomain, String upDataDomain, String upMeatDomain)
            throws Exception {
        // create region
        CreateRegionRequest request = new CreateRegionRequest(regionName);
        request.withDataCSShardingType(DataShardingType.YEAR).withDataCLShardingType(DataShardingType.YEAR)
                .withDataDomain(dataDomain).withMetaDomain(metaDomain);
        regionClient.createRegion(request);

        request.withDataCSShardingType(DataShardingType.YEAR).withDataCLShardingType(DataShardingType.YEAR)
                .withDataDomain(upDataDomain).withMetaDomain(upMeatDomain);
        try {
            regionClient.createRegion(request);
            Assert.fail("exp failed but act success,region = " + request.getRegion().toString());
        } catch (SequoiaS3ServiceException e) {
            if (e.getStatusCode() != 409 && !e.getErrorCode().contains("ConflictDomain")) {
                throw e;
            }
        }
        GetRegionResult result = regionClient.getRegion(regionName);
        Assert.assertEquals(result.getRegion().getDataDomain(), dataDomain);
        Assert.assertEquals(result.getRegion().getMetaDomain(), metaDomain);
        actSuccessTests.getAndIncrement();
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if (actSuccessTests.get() == rangeData().length) {
                for (String regionName : regionNames) {
                    regionClient.deleteRegion(regionName);
                }
                RegionUtils.dropDomain(existDomain);
            }
        } finally {
            regionClient.shutdown();
        }
    }
}
