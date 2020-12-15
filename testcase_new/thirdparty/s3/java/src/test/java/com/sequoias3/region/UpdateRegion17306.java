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
 * @author fanyu
 * @Description: seqDB-17306 :: 更新区域配置domain
 * @Date:2019年01月22日
 * @version:1.0
 */
public class UpdateRegion17306 extends S3TestBase {
    private String[] domainNames = { "domain17306A", "domain17306B" };
    private String[] regionNames = { "region17306a", "region17306b", "region17306c" };
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
        for (String domainName : domainNames) {
            RegionUtils.createDomain(domainName);
        }
        return new Object[][] {
                // regionName dataDomain metaDomain updateDataDomain
                // updateMeatDomain
                { regionNames[0], domainNames[0], domainNames[1], domainNames[1], domainNames[1] },
                { regionNames[1], domainNames[0], domainNames[1], domainNames[0], domainNames[0] },
                { regionNames[2], domainNames[0], domainNames[1], domainNames[0], domainNames[1] }, };
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
        // Updated domain is not same as before when the index of regionName is
        // not equal to 2
        if (!regionName.equals(regionNames[2])) {
            try {
                // update region
                regionClient.createRegion(request);
                Assert.fail("exp failed but act success,region = " + request.getRegion().toString());
            } catch (SequoiaS3ServiceException e) {
                if (e.getStatusCode() != 409 && !e.getErrorCode().contains("ConflictDomain")) {
                    throw e;
                }
            }
        } else {
            // update region
            regionClient.createRegion(request);
            GetRegionResult result = regionClient.getRegion(regionName);
            Assert.assertEquals(result.getRegion().getDataDomain(), upDataDomain);
            Assert.assertEquals(result.getRegion().getMetaDomain(), upMeatDomain);
        }
        actSuccessTests.getAndIncrement();
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if (actSuccessTests.get() == rangeData().length) {
                for (String regionName : regionNames) {
                    regionClient.deleteRegion(regionName);
                }
                for (String domainName : domainNames) {
                    RegionUtils.dropDomain(domainName);
                }
            }
        } finally {
            regionClient.shutdown();
        }

    }
}
