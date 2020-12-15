package com.sequoias3.config.concurrent;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.amazonaws.services.s3.AmazonS3;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;
import com.sequoias3.SequoiaS3;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.RegionUtils;
import com.sequoias3.testcommon.s3utils.UserUtils;
import com.sequoias3.testcommon.s3utils.bean.UserCommDefind;

/**
 * @Description seqDB-18614: 配置允许重复创建桶，同一用户并发创建相同桶
 *
 * @author wangkexin
 * @Date 2019.06.25
 * @version 1.00
 */
@Test(groups = "allowreputon")
public class CreateSameBucket18614 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket18614";
    private String userName = "user18614";
    private String[] accessKeys = null;
    private String keyName = "key18614";
    private String content = "content18614";
    private String regionName = "region18614a";
    private String regionName2 = "region18614b";
    private int threadNum = 100;
    private AmazonS3 s3Client = null;
    private SequoiaS3 regionClient = null;

    @BeforeClass
    private void setUp() throws Exception {
        CommLib.clearUser(userName);
        regionClient = CommLib.regionClient();
        RegionUtils.clearRegion(regionClient, regionName);
        RegionUtils.clearRegion(regionClient, regionName2);
        accessKeys = UserUtils.createUser(userName, UserCommDefind.normal);
        s3Client = CommLib.buildS3Client(accessKeys[0], accessKeys[1]);

        regionClient.createRegion(regionName);
        regionClient.createRegion(regionName2);
    }

    @SuppressWarnings("deprecation")
    @Test
    private void testReputBacket() throws Exception {
        s3Client.createBucket(bucketName, regionName);

        ThreadExecutor es = new ThreadExecutor();
        for (int i = 0; i < threadNum; i++) {
            es.addWorker(new ThreadCreateBucket18614());
        }
        es.run();

        // check bucket list and bucket info
        Assert.assertEquals(s3Client.listBuckets().size(), 1);
        Assert.assertTrue(s3Client.doesBucketExist(bucketName));
        Assert.assertEquals(s3Client.getBucketLocation(bucketName), regionName);

        checkBucket();
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if (runSuccess) {
                CommLib.clearUser(userName);
                regionClient.deleteRegion(regionName);
                regionClient.deleteRegion(regionName2);
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

    private void checkBucket() {
        s3Client.putObject(bucketName, keyName, content);
        String actEtg = s3Client.getObject(bucketName, keyName).getObjectMetadata().getETag();
        Assert.assertEquals(actEtg, TestTools.getMD5(content.getBytes()));
    }

    class ThreadCreateBucket18614 {
        private AmazonS3 s3Client = CommLib.buildS3Client(accessKeys[0], accessKeys[1]);

        @SuppressWarnings("deprecation")
        @ExecuteOrder(step = 1, desc = "上传对象")
        public void putObject() {
            try {
                s3Client.createBucket(bucketName, regionName2);
            } finally {
                if (s3Client != null) {
                    s3Client.shutdown();
                }
            }
        }
    }
}
