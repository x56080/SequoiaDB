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
 * @Description seqDB-17328:Head查询区域信息
 * @author wangkexin
 * @Date 2019.01.25
 * @version 1.00
 */

public class HeadRegion17328 extends S3TestBase {
    private String regionName = "Beijing17328";
    private String bucketName = "bucket17328";
    private AmazonS3 s3Client = null;
    private SequoiaS3 regionClient = null;

    @BeforeClass
    private void setUp() throws Exception {
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        regionClient = CommLib.regionClient();
        RegionUtils.clearRegion( regionClient, regionName );

        // create region
        regionClient.createRegion( regionName );
        s3Client.createBucket( new CreateBucketRequest( bucketName,
                regionName.toLowerCase() ) );
        s3Client.putObject( bucketName, "key17328", "content17328" );
    }

    @Test
    public void testCreateRegion() throws Exception {
        Assert.assertTrue( regionClient.headRegion( regionName ) );
        CommLib.clearBucket( s3Client, bucketName );
        regionClient.deleteRegion( regionName );
        Assert.assertFalse( regionClient.headRegion( regionName ) );
    }

    @AfterClass
    private void tearDown() throws Exception {
        regionClient.shutdown();
        s3Client.shutdown();

    }
}
