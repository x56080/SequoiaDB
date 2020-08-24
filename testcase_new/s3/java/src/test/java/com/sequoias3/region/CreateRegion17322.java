package com.sequoias3.region;

import java.util.ArrayList;
import java.util.List;

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
 * @Description: seqDB-17322 :: 区域中创建/删除多个桶，获取区域信息
 * @author fanyu
 * @Date:2019年01月22日
 * @version:1.0
 */
public class CreateRegion17322 extends S3TestBase {
    private String regionName = "region17322";
    private String bucketNameBase = "bucket17322";
    private List< String > bucketNames = new ArrayList< String >();
    private int bucketNum = 80;
    private AmazonS3 s3Client = null;
    private boolean runSuccess = false;
    private SequoiaS3 regionClient = null;

    @BeforeClass
    private void setUp() throws Exception {
        s3Client = CommLib.buildS3Client();
        regionClient = CommLib.regionClient();
        RegionUtils.clearRegion( regionClient, regionName );
        for ( int i = 0; i < bucketNum; i++ ) {
            bucketNames.add( bucketNameBase + "-" + i );
        }
        for ( String bucketName : bucketNames ) {
            CommLib.clearBucket( s3Client, bucketName );
        }
    }

    @Test
    private void test() throws Exception {
        // create region
        CreateRegionRequest request = new CreateRegionRequest( regionName );
        request.withDataCSShardingType( DataShardingType.YEAR )
                .withDataCLShardingType( DataShardingType.YEAR );
        regionClient.createRegion( request );

        // create bucket
        for ( String bucketName : bucketNames ) {
            s3Client.createBucket(
                    new CreateBucketRequest( bucketName, regionName ) );
        }

        GetRegionResult result = regionClient.getRegion( regionName );
        checkGetRegionResult( result, bucketNames );

        // delete bockets
        for ( int i = bucketNum / 2; i < ( bucketNum * 3 ) / 4; i++ ) {
            s3Client.deleteBucket( bucketNames.get( i ) );
            bucketNames.remove( i );
        }

        // get region info again
        GetRegionResult result1 = regionClient.getRegion( regionName );
        checkGetRegionResult( result1, bucketNames );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( runSuccess ) {
                for ( String bucketName : bucketNames ) {
                    CommLib.clearBucket( s3Client, bucketName );
                }
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

    private void checkGetRegionResult( GetRegionResult result,
            List< String > expBucketNames ) throws Exception {
        Region actRegion = result.getRegion();
        Assert.assertEquals( actRegion.getDataCSShardingType().toString(),
                "year" );
        Assert.assertEquals( actRegion.getDataCLShardingType().toString(),
                "year" );
        Assert.assertEquals( actRegion.getDataLocation(), null );
        Assert.assertEquals( actRegion.getMetaLocation(), null );
        Assert.assertEquals( actRegion.getMetaHisLocation(), null );
        Assert.assertEquals( actRegion.getMetaDomain(), null );
        Assert.assertEquals( actRegion.getDataDomain(), null );
        List< String > actBuckets = result.getBuckets();
        Assert.assertEquals( actBuckets.size(), expBucketNames.size(),
                "actBuckets = " + actBuckets.toString() + ",expBucketNames = "
                        + expBucketNames.toString() );
        for ( int i = 0; i < actBuckets.size(); i++ ) {
            if ( !expBucketNames.contains( actBuckets.get( i ) ) ) {
                throw new Exception( "exp bucketName not in act BuckNames"
                        + ",actBucket = " + actBuckets.toString()
                        + ",expBuckets = " + expBucketNames.toString() );
            }
        }
    }
}
