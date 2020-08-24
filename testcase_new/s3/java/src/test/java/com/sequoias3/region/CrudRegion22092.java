package com.sequoias3.region;

import java.util.UUID;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.S3Object;
import com.sequoias3.SequoiaS3;
import com.sequoias3.common.DataShardingType;
import com.sequoias3.exception.SequoiaS3ServiceException;
import com.sequoias3.model.CreateRegionRequest;
import com.sequoias3.model.GetRegionResult;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.RegionUtils;

/**
 * @Description: seqDB-22092:自动创建模式配置DataLobPageSize和DataReplSize ，创建/列取/获取/更新/删除区域
 * @author fanyu
 * @Date:2019年04月21日
 * @version:1.0
 */
public class CrudRegion22092 extends S3TestBase {
    DataShardingType dataCSShardingType = DataShardingType.QUARTER;
    DataShardingType dataCLShardingType = DataShardingType.MONTH;
    private String regionName = "region22092";
    private String bucketName = "bucket22092";
    private String objectName = "object22092";
    private AmazonS3 s3Client = null;
    private SequoiaS3 regionClient = null;

    @BeforeClass
    private void setUp() throws Exception {
        s3Client = CommLib.buildS3Client();
        regionClient = CommLib.regionClient();
        RegionUtils.clearRegion(regionClient, regionName);
    }

    @DataProvider(name = "data-provier")
    private Object[][] dataProvier() {
        return new Object[][] {
                // DataLobPageSize DataReplSize
                { 0, -1 }, { 4096, 0 }, { 8192, 1 }, { 16384, 2 }, { 32768, 3 }, { 65536, 4 }, { 131072, 5 },
                { 262144, 6 }, { 524288, 7 } };
    }

    @Test(dataProvider = "data-provier")
    private void test(int dataLobPageSize, int replSize) throws Exception {
        CreateRegionRequest request = new CreateRegionRequest(regionName);
        request.withDataCSShardingType(dataCSShardingType).withDataCLShardingType(dataCLShardingType)
                .withDataLobPageSize(dataLobPageSize).withDataReplSize(replSize);
        // put region
        regionClient.createRegion(request);

        // craete bucket for check
        s3Client.createBucket(new CreateBucketRequest(bucketName, regionName));
        // create object for check
        s3Client.putObject(bucketName, objectName, String.valueOf(UUID.randomUUID()));
        // get object for check
        S3Object s3Object = s3Client.getObject(bucketName, objectName);
        Assert.assertEquals(s3Object.getBucketName(), bucketName);
        Assert.assertEquals(s3Object.getKey(), objectName);
        Assert.assertEquals(s3Object.getObjectMetadata().getVersionId(), "null");

        // get region
        GetRegionResult result1 = regionClient.getRegion(regionName);

        Assert.assertEquals(result1.getRegion().getDataLobPageSize().intValue(), dataLobPageSize, result1.toString());
        Assert.assertEquals(result1.getRegion().getDataReplSize().intValue(), replSize, result1.toString());

        // update region
        // the same as before
        CreateRegionRequest updateRegion1 = new CreateRegionRequest(regionName);
        updateRegion1.withDataCSShardingType(dataCSShardingType).withDataCLShardingType(dataCLShardingType)
                .withDataLobPageSize(dataLobPageSize).withDataReplSize(replSize);
        regionClient.createRegion(updateRegion1);

        // update failed
        CreateRegionRequest updateRegion2 = new CreateRegionRequest(regionName);
        updateRegion2.withDataCSShardingType(dataCSShardingType).withDataCLShardingType(dataCLShardingType)
                .withDataLobPageSize(1024).withDataReplSize(8);
        try {
            regionClient.createRegion(updateRegion2);
            Assert.fail("exp failed but act success!!!");
        } catch (SequoiaS3ServiceException e) {
            if (e.getStatusCode() != 409) {
                throw e;
            }
        }
        // check
        GetRegionResult result2 = regionClient.getRegion(regionName);
        Assert.assertEquals(result2.getRegion().getDataLobPageSize().intValue(), dataLobPageSize, result2.toString());
        Assert.assertEquals(result2.getRegion().getDataReplSize().intValue(), replSize, result2.toString());

        // clean
        CommLib.clearBucket(s3Client, bucketName);
        regionClient.deleteRegion(regionName);
        Assert.assertFalse(regionClient.headRegion(regionName));
    }

    @AfterClass
    private void tearDown() {
        regionClient.shutdown();
        if (s3Client != null) {
            s3Client.shutdown();
        }
    }
}
