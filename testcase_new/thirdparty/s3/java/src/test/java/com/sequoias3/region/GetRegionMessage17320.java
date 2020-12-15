package com.sequoias3.region;

import java.util.Date;
import java.util.List;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.sequoiadb.base.Sequoiadb;
import com.sequoias3.SequoiaS3;
import com.sequoias3.common.DataShardingType;
import com.sequoias3.model.CreateRegionRequest;
import com.sequoias3.model.GetRegionResult;
import com.sequoias3.model.Region;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.RegionUtils;

/**
 * @Description seqDB-17320:获取区域信息
 * @author wangkexin
 * @Date 2019.01.24
 * @version 1.00
 */

public class GetRegionMessage17320 extends S3TestBase {
    private static Sequoiadb sdb = null;
    String shardingDataCSName = null;
    String shardingMetaCSName = null;
    private String specifiedModeRegion = "Beijing17320";
    private String shardingModeRegion = "Shanghai17320";
    private String dataDomain = "dataDomain17320";
    private String metaDomain = "metaDomain17320";
    private String metaCSName = "metaCS17320";
    private String dataCSName = "dataCS17320";
    private String[] metaClNames = { "metaCL17320", "metaHistoryCL17320" };
    private String[] dataClName = { "dataCL17320" };
    private String spmodeBucket = "bucket17320-1";
    private String shmodeBucket = "bucket17320-2";
    private AmazonS3 s3Client = null;
    private boolean runSuccess = false;
    private SequoiaS3 regionClient = null;

    @BeforeClass
    private void setUp() throws Exception {
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket(s3Client, spmodeBucket);
        CommLib.clearBucket(s3Client, shmodeBucket);

        // delete data/meta cs
        shardingDataCSName = RegionUtils.getDataCSName(shardingModeRegion.toLowerCase(), DataShardingType.QUARTER,
                new Date()) + "_1";
        shardingMetaCSName = RegionUtils.getMetaCSName(shardingModeRegion.toLowerCase());
        sdb = new Sequoiadb(S3TestBase.coordUrl, "", "");
        if (sdb.isCollectionSpaceExist(shardingDataCSName)) {
            sdb.dropCollectionSpace(shardingDataCSName);
        }
        if (sdb.isCollectionSpaceExist(shardingMetaCSName)) {
            sdb.dropCollectionSpace(shardingMetaCSName);
        }

        RegionUtils.dropDomain(metaDomain);
        RegionUtils.dropDomain(dataDomain);
        regionClient = CommLib.regionClient();
        RegionUtils.clearRegion(regionClient, specifiedModeRegion);
        RegionUtils.clearRegion(regionClient, shardingModeRegion);

        RegionUtils.createCSAndCL(metaCSName, metaClNames);
        RegionUtils.createCSAndCL(dataCSName, dataClName);

        RegionUtils.createDomain(dataDomain);
        RegionUtils.createDomain(metaDomain);

        // create region with specified mode
        CreateRegionRequest request = new CreateRegionRequest(specifiedModeRegion);
        request.withMetaLocation(metaCSName + "." + metaClNames[0])
                .withMetaHisLocation(metaCSName + "." + metaClNames[1])
                .withDataLocation(dataCSName + "." + dataClName[0]);
        regionClient.createRegion(request);

        // create region with ShardingType mode
        CreateRegionRequest request1 = new CreateRegionRequest(shardingModeRegion);
        request1.withDataCSShardingType(DataShardingType.QUARTER).withDataCLShardingType(DataShardingType.MONTH)
                .withDataDomain(dataDomain).withMetaDomain(metaDomain);
        regionClient.createRegion(request1);

        // create bucket and put object
        s3Client.createBucket(new CreateBucketRequest(spmodeBucket, specifiedModeRegion.toLowerCase()));
        s3Client.createBucket(new CreateBucketRequest(shmodeBucket, shardingModeRegion.toLowerCase()));

        s3Client.putObject(spmodeBucket, "key17320_1", "content17320_1");
        s3Client.putObject(shmodeBucket, "key17320_2", "content17320_2");
    }

    @Test
    public void testCreateRegion() throws Exception {
        // specified mode : get region
        GetRegionResult spResult = regionClient.getRegion(specifiedModeRegion);
        Region spRegion = spResult.getRegion();
        Assert.assertEquals(spRegion.getName(), specifiedModeRegion.toLowerCase());
        Assert.assertEquals(spRegion.getMetaLocation(), metaCSName + "." + metaClNames[0]);
        Assert.assertEquals(spRegion.getMetaHisLocation(), metaCSName + "." + metaClNames[1]);
        Assert.assertEquals(spRegion.getDataLocation(), dataCSName + "." + dataClName[0]);
        List<String> buckets = spResult.getBuckets();
        Assert.assertEquals(buckets.get(0), spmodeBucket);

        // ShardingType mode : get region
        GetRegionResult stResult = regionClient.getRegion(shardingModeRegion);
        Region stRegion = stResult.getRegion();
        Assert.assertEquals(stRegion.getName(), shardingModeRegion.toLowerCase());
        Assert.assertEquals(stRegion.getDataCSShardingType(), DataShardingType.QUARTER);
        Assert.assertEquals(stRegion.getDataCLShardingType(), DataShardingType.MONTH);
        Assert.assertEquals(stRegion.getMetaDomain(), metaDomain);
        Assert.assertEquals(stRegion.getDataDomain(), dataDomain);
        List<String> buckets2 = stResult.getBuckets();
        Assert.assertEquals(buckets2.get(0), shmodeBucket);
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if (runSuccess) {
                CommLib.clearBucket(s3Client, spmodeBucket);
                CommLib.clearBucket(s3Client, shmodeBucket);
                sdb.dropCollectionSpace(metaCSName);
                sdb.dropCollectionSpace(dataCSName);
                sdb.dropCollectionSpace(shardingDataCSName);
                sdb.dropCollectionSpace(shardingMetaCSName);
                RegionUtils.dropDomain(dataDomain);
                RegionUtils.dropDomain(metaDomain);
                regionClient.deleteRegion(specifiedModeRegion);
                regionClient.deleteRegion(shardingModeRegion);
            }
        } finally {
            if (sdb != null) {
                sdb.close();
            }
            if (regionClient != null) {
                regionClient.shutdown();
            }
            if (s3Client != null) {
                s3Client.shutdown();
            }
        }
    }
}