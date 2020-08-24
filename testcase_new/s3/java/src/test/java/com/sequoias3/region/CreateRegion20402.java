package com.sequoias3.region;

import java.util.concurrent.atomic.AtomicInteger;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.S3Object;
import com.amazonaws.util.Md5Utils;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;
import com.sequoias3.SequoiaS3;
import com.sequoias3.model.CreateRegionRequest;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.RegionUtils;

/**
 * @Description seqDB-20402:创建区域使用指定模式，设置csRange
 * @author fanyu
 * @Date:2020年01月03日
 * @version 1.00
 */
public class CreateRegion20402 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket20402";
    private String objectNameBase = "object20402-";
    private int objectNum = 20;
    private AtomicInteger counter = new AtomicInteger(objectNum);
    private String regionName = "region20402";
    private AmazonS3 s3Client = null;
    private String[] csNames = { "metaCS20402", "dataCS20402" };
    private String[] metaclNames = { "metaCL20402", "metaHistroyCL20402" };
    private String[] dataclNames = { "dataCL20402" };
    private SequoiaS3 regionClient = null;

    @BeforeClass
    private void setUp() throws Exception {
        RegionUtils.createCSAndCL(csNames[0], metaclNames);
        RegionUtils.createCSAndCL(csNames[1], dataclNames);
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket(s3Client, bucketName);
        regionClient = CommLib.regionClient();
        RegionUtils.clearRegion(regionClient, regionName);
    }

    @Test
    @SuppressWarnings("deprecation")
    public void testRegion() throws Exception {
        String metaLocation = csNames[0] + "." + metaclNames[0];
        String metaHisLocation = csNames[0] + "." + metaclNames[1];
        String dataLocation = csNames[1] + "." + dataclNames[0];
        CreateRegionRequest request = new CreateRegionRequest(regionName);
        request.withMetaLocation(metaLocation).withDataLocation(dataLocation).withMetaHisLocation(metaHisLocation)
                .withDataCSRange(10);
        regionClient.createRegion(request);
        // create bucket
        s3Client.createBucket(bucketName, regionName);
        // create object on region
        ThreadExecutor executor = new ThreadExecutor();
        for (int i = 0; i < objectNum; i++) {
            executor.addWorker(new CreateObject());
        }
        executor.run();
        // check result
        int actCount = RegionUtils.getRecordNum(csNames[1], dataclNames[0]);
        Assert.assertEquals(actCount, objectNum);
        for (int i = 1; i <= objectNum; i++) {
            S3Object obj = s3Client.getObject(bucketName, objectNameBase + i);
            Assert.assertEquals(Md5Utils.md5AsBase64(obj.getObjectContent()),
                    Md5Utils.md5AsBase64(String.valueOf(i).getBytes()),
                    "bucketName = " + bucketName + ",objectName = " + objectNameBase + i);
        }
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if (runSuccess) {
                CommLib.clearBucket(s3Client, bucketName);
                regionClient.deleteRegion(regionName);
                RegionUtils.dropCS(csNames);
            }
        } finally {
            regionClient.shutdown();
            s3Client.shutdown();
        }
    }

    class CreateObject {
        @ExecuteOrder(step = 1)
        public void createObject() {
            AmazonS3 s3Client = CommLib.buildS3Client();
            try {
                int i = counter.getAndDecrement();
                s3Client.putObject(bucketName, objectNameBase + i, String.valueOf(i));
            } finally {
                s3Client.shutdown();
            }
        }
    }
}
