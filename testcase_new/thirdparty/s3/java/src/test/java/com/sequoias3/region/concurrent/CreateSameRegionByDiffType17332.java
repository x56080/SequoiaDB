package com.sequoias3.region.concurrent;

import java.util.HashMap;
import java.util.Map;
import java.util.UUID;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.S3Object;
import com.sequoias3.SequoiaS3;
import com.sequoias3.common.DataShardingType;
import com.sequoias3.model.CreateRegionRequest;
import com.sequoias3.model.GetRegionResult;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.S3ThreadBase;
import com.sequoias3.testcommon.s3utils.RegionUtils;

/**
 * @Description: seqDB-17332 :: 并发创建相同区域（配置不同）
 * @author fanyu
 * @Date:2019年01月22日
 * @version:1.0
 */
public class CreateSameRegionByDiffType17332 extends S3TestBase {
    private boolean runSuccess = false;
    private String regionName = "region17332";
    private String bucketName = "bucket17332";
    private String objectName = "object17332";
    private AmazonS3 s3Client = null;
    private SequoiaS3 regionClient = null;

    @BeforeClass
    private void setUp() throws Exception {
        s3Client = CommLib.buildS3Client();
        regionClient = CommLib.regionClient();
        RegionUtils.clearRegion(regionClient, regionName);
    }

    @Test
    private void test() throws Exception {
        Map<String, DataShardingType> hashMap1 = new HashMap<String, DataShardingType>();
        hashMap1.put("dataCSShardingType", DataShardingType.MONTH);
        hashMap1.put("dataCLShardingType", DataShardingType.QUARTER);

        Map<String, DataShardingType> hashMap2 = new HashMap<String, DataShardingType>();
        hashMap2.put("dataCSShardingType", DataShardingType.QUARTER);
        hashMap2.put("dataCLShardingType", DataShardingType.YEAR);

        CreateRegion cThread1 = new CreateRegion(hashMap1.get("dataCSShardingType"),
                hashMap1.get("dataCLShardingType"));
        CreateRegion cThread2 = new CreateRegion(hashMap2.get("dataCSShardingType"),
                hashMap2.get("dataCLShardingType"));
        cThread1.start(20);
        cThread2.start(20);
        Assert.assertEquals(cThread1.isSuccess(), true, cThread1.getErrorMsg());
        Assert.assertEquals(cThread2.isSuccess(), true, cThread2.getErrorMsg());

        // get region
        GetRegionResult region = regionClient.getRegion(regionName);
        Map<String, DataShardingType> actMap = new HashMap<String, DataShardingType>();
        actMap.put("dataCSShardingType", region.getRegion().getDataCSShardingType());
        actMap.put("dataCLShardingType", region.getRegion().getDataCLShardingType());
        // check region sharding type
        if (!actMap.toString().equals(hashMap1.toString()) && !actMap.toString().equals(hashMap2.toString())) {
            throw new Exception("actMap = " + actMap.toString() + ",hashMap1 = " + hashMap1.toString() + ",hashMap2 = "
                    + hashMap2.toString());
        }

        // craete bucket for check
        s3Client.createBucket(new CreateBucketRequest(bucketName, regionName));
        // create object for check
        s3Client.putObject(bucketName, objectName, String.valueOf(UUID.randomUUID()));
        // get object for check
        S3Object s3Object = s3Client.getObject(bucketName, objectName);
        Assert.assertEquals(s3Object.getBucketName(), bucketName);
        Assert.assertEquals(s3Object.getKey(), objectName);
        Assert.assertEquals(s3Object.getObjectMetadata().getVersionId(), "null");
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if (runSuccess) {
                CommLib.clearBucket(s3Client, bucketName);
                regionClient.deleteRegion(regionName);
            }
        } finally {
            regionClient.shutdown();
        }

    }

    private class CreateRegion extends S3ThreadBase {
        private DataShardingType dataCSShardingType;
        private DataShardingType dataCLShardingType;

        public CreateRegion(DataShardingType dataCSShardingType, DataShardingType dataCLShardingType) {
            this.dataCSShardingType = dataCSShardingType;
            this.dataCLShardingType = dataCLShardingType;
        }

        @Override
        public void exec() throws Exception {
            SequoiaS3 regionClient1 = CommLib.regionClient();
            try {
                CreateRegionRequest request = new CreateRegionRequest(regionName);
                request.withDataCSShardingType(this.dataCSShardingType).withDataCLShardingType(this.dataCLShardingType);
                regionClient1.createRegion(request);
            } finally {
                regionClient1.shutdown();
            }

        }
    }
}
