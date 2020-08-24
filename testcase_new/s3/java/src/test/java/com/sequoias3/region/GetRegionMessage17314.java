package com.sequoias3.region;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.sequoias3.SequoiaS3;
import com.sequoias3.common.DataShardingType;
import com.sequoias3.model.CreateRegionRequest;
import com.sequoias3.model.GetRegionResult;
import com.sequoias3.model.Region;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.RegionUtils;

/**
 * @Description seqDB-17314:桶所在区域已更新，获取区域信息 *
 * @author wangkexin
 * @Date 2019.01.23
 * @version 1.00
 */

public class GetRegionMessage17314 extends S3TestBase {
    private AmazonS3 s3Client = null;
    private String bucketName = "bucket17314";
    private String regionName = "beijing17314";
    private boolean runSuccess = false;
    private SequoiaS3 regionClient = null;

    @BeforeClass
    private void setUp() throws Exception {
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket(s3Client, bucketName);
        regionClient = CommLib.regionClient();
        RegionUtils.clearRegion(regionClient, regionName);
    }

    @Test
    public void testGetRegionMessage() throws Exception {
        // create region
        regionClient.createRegion(regionName);
        s3Client.createBucket(new CreateBucketRequest(bucketName, regionName));
        // update region
        CreateRegionRequest request = new CreateRegionRequest(regionName);
        request.withDataCSShardingType(DataShardingType.MONTH);
        regionClient.createRegion(request);

        // check result
        GetRegionResult result = regionClient.getRegion(regionName);
        Region currRegion = result.getRegion();
        Assert.assertEquals(result.getBuckets().get(0), bucketName);
        Assert.assertEquals(currRegion.getName(), regionName);
        Assert.assertEquals(currRegion.getDataCSShardingType(), DataShardingType.MONTH);
        Assert.assertEquals(currRegion.getDataCLShardingType(), DataShardingType.QUARTER);

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if (runSuccess) {
                s3Client.deleteBucket(bucketName);
                regionClient.deleteRegion(regionName);
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
}
