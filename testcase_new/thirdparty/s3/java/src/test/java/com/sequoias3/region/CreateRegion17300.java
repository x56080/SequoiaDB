package com.sequoias3.region;

import java.util.ArrayList;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.atomic.AtomicInteger;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.sequoias3.SequoiaS3;
import com.sequoias3.common.DataShardingType;
import com.sequoias3.exception.SequoiaS3ClientException;
import com.sequoias3.exception.SequoiaS3ServiceException;
import com.sequoias3.model.CreateRegionRequest;
import com.sequoias3.model.GetRegionResult;
import com.sequoias3.model.Region;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.RegionUtils;

/**
 * @Description: seqDB-17300 :: 创建区域配置domain
 * @author fanyu
 * @Date:2019年01月22日
 * @version:1.0
 */
public class CreateRegion17300 extends S3TestBase {
    private String bucketName = "bucket17300";
    private String objectName = "object17300";
    private String[] domainNames = { "domain17300A", "domain17300B" };
    private String[] regionNames = { "region17300a", "region17300b", "region17300c", "region17300d" };
    private AmazonS3 s3Client = null;
    private SequoiaS3 regionClient = null;
    private AtomicInteger actSuccessTests = new AtomicInteger(0);

    @BeforeClass
    private void setUp() throws Exception {
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket(s3Client, bucketName);
        regionClient = CommLib.regionClient();
        for (String regionName : regionNames) {
            RegionUtils.clearRegion(regionClient, regionName);
        }
    }

    @DataProvider(name = "range-provider")
    private Object[][] rangeData() {
        for (String domainName : domainNames) {
            RegionUtils.createDomain(domainName);
        }
        return new Object[][] {
                // regionName dataDomain metaDomain expDataDomain expMetaDomain
                { regionNames[0], domainNames[0], null, domainNames[0], "" },
                { regionNames[1], null, domainNames[1], null, domainNames[1] },
                { regionNames[2], domainNames[1], domainNames[1], domainNames[1], domainNames[1] },
                { regionNames[3], domainNames[0], domainNames[1], domainNames[0], domainNames[1] }, };
    }

    @Test(dataProvider = "range-provider")
    private void test(String regionName, String dataDomain, String metaDomain, String expDataDomain,
            String expMeatDomain) throws Exception {
        // create region
        CreateRegionRequest request = new CreateRegionRequest(regionName);
        request.withDataCSShardingType(DataShardingType.YEAR).withDataCLShardingType(DataShardingType.YEAR)
                .withDataDomain(dataDomain).withMetaDomain(metaDomain);
        regionClient.createRegion(request);

        // create bucket for check
        String randomBucketName = bucketName + "-" + UUID.randomUUID();
        s3Client.createBucket(new CreateBucketRequest(randomBucketName, regionName));
        s3Client.putObject(randomBucketName, objectName, String.valueOf(UUID.randomUUID()));

        // expResult
        List<String> expBuckets = new ArrayList<>();
        expBuckets.add(randomBucketName);
        // check
        checkGetResult(regionName, expDataDomain, expMeatDomain, expBuckets);
        CommLib.clearBucket(s3Client, randomBucketName);
        regionClient.deleteRegion(regionName);
        actSuccessTests.getAndIncrement();
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if (actSuccessTests.get() == rangeData().length) {
                for (String domainName : domainNames) {
                    RegionUtils.dropDomain(domainName);
                }
            }
        } finally {
            if (regionClient != null) {
                regionClient.shutdown();
            }
            if (s3Client != null) {
                s3Client.shutdown();
            }
        }
    }

    private void checkGetResult(String regionName, String expDataDomain, String expMetaDomain, List<String> expBuckets)
            throws SequoiaS3ServiceException, SequoiaS3ClientException {
        if ("".equals(expMetaDomain)) {
            expMetaDomain = null;
        }
        GetRegionResult result = regionClient.getRegion(regionName);
        Region regionInfo = result.getRegion();
        Assert.assertEquals(regionInfo.getDataCLShardingType(), DataShardingType.YEAR);
        Assert.assertEquals(regionInfo.getDataCSShardingType(), DataShardingType.YEAR);
        Assert.assertEquals(regionInfo.getMetaDomain(), expMetaDomain);
        Assert.assertEquals(regionInfo.getDataDomain(), expDataDomain);
        Assert.assertEquals(regionInfo.getMetaLocation(), null);
        Assert.assertEquals(regionInfo.getMetaHisLocation(), null);
        Assert.assertEquals(regionInfo.getDataLocation(), null);
        Assert.assertEquals(regionInfo.getDataLobPageSize().intValue(), 262144);
        Assert.assertEquals(regionInfo.getDataReplSize().intValue(), -1);
        List<String> buckets = result.getBuckets();

        Assert.assertEquals(buckets.size(), expBuckets.size());
        for (int i = 0; i < expBuckets.size(); i++) {
            Assert.assertEquals(buckets.get(i), expBuckets.get(i));
        }
    }
}
