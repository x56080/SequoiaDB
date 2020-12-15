package com.sequoias3.region;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.sequoias3.SequoiaS3;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.RegionUtils;

/**
 * @Description seqDB-17357:GetBucketLocation接口参数校验
 * @author wangkexin
 * @Date 2019.01.24
 * @version 1.00
 */

public class TestGetBucketLocation17357 extends S3TestBase {
    private String regionName = "beijing17357";
    private String bucketName = "bucket17357";
    private AmazonS3 s3Client = null;
    private boolean runSuccess = false;
    private SequoiaS3 regionClient = null;

    @BeforeClass
    private void setUp() throws Exception {
        regionClient = CommLib.regionClient();
        RegionUtils.clearRegion( regionClient, regionName );

        regionClient.createRegion( regionName );
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket(
                new CreateBucketRequest( bucketName, regionName ) );
    }

    @Test
    public void testCreateRegion() throws Exception {
        // 合法值
        Assert.assertEquals( s3Client.getBucketLocation( bucketName ),
                regionName );

        // 非法值,aws的sdk里面已做处理，为空时对外返回US
        Assert.assertEquals( s3Client.getBucketLocation( "" ), "US" );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( runSuccess ) {

                CommLib.clearBucket( s3Client, bucketName );
                regionClient.deleteRegion( regionName );
            }
        } finally {
            if ( regionClient != null ) {
                regionClient.shutdown();
            }
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }
}
