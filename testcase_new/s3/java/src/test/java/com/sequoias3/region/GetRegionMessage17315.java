package com.sequoias3.region;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.amazonaws.services.s3.AmazonS3;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;

/**
 * @Description seqDB-17315:创建桶不指定区域，获取区域信息
 * @author wangkexin
 * @Date 2019.01.24
 * @version 1.00
 */

public class GetRegionMessage17315 extends S3TestBase {
    private AmazonS3 s3Client = null;
    private String bucketName = "bucket17315";
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket(s3Client, bucketName);
    }

    @Test
    public void testGetRegionMessage() throws Exception {
        s3Client.createBucket(bucketName);
        String str = s3Client.getBucketLocation(bucketName);
        Assert.assertEquals(str, "US");
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if (runSuccess) {
                s3Client.deleteBucket(bucketName);
            }
        } finally {
            if (s3Client != null) {
                s3Client.shutdown();
            }
        }
    }
}
