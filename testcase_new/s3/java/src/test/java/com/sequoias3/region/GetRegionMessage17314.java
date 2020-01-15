package com.sequoias3.region;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.bean.GetRegionResult;
import com.sequoias3.testcommon.s3utils.bean.Region;
import com.sequoias3.testcommon.s3utils.RegionUtils;

/**
 * test content: 桶所在区域已更新，获取区域信息 testlink-case: seqDB-17314
 *
 * @author wangkexin
 * @Date 2019.01.23
 * @version 1.00
 */

public class GetRegionMessage17314 extends S3TestBase {
    private AmazonS3 s3Client = null;
    private String bucketName = "bucket17314";
    private String regionName = "beijing17314";
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {
        s3Client = CommLib.buildS3Client();

        CommLib.clearBucket( s3Client, bucketName );
        RegionUtils.clearRegion( regionName );
    }

    @Test
    public void testGetRegionMessage() throws Exception {
        // create region
        Region region = new Region();
        region.withName( regionName );
        RegionUtils.putRegion( region );

        s3Client.createBucket(
                new CreateBucketRequest( bucketName, regionName ) );
        // update region
        Region newRegion = new Region();
        newRegion.withName( regionName ).withDataCSShardingType( "month" );
        RegionUtils.putRegion( newRegion );

        // check result
        GetRegionResult result = RegionUtils.getRegion( regionName );
        Region currRegion = result.getRegion();
        Assert.assertEquals( result.getBuckets().get( 0 ).getName(),
                bucketName );
        Assert.assertEquals( currRegion.getName(), regionName );
        Assert.assertEquals( currRegion.getDataCSShardingType(), "month" );
        Assert.assertEquals( currRegion.getDataCLShardingType(), "quarter" );

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( runSuccess ) {
                s3Client.deleteBucket( bucketName );
                RegionUtils.deleteRegion( regionName );
            }
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }
}
