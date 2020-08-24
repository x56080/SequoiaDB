package com.sequoias3.region.concurrent;

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
import com.sequoias3.model.Region;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.S3ThreadBase;
import com.sequoias3.testcommon.s3utils.RegionUtils;

/**
 * @Description: seqDB-17334 :: 并发更新相同区域（配置不同）
 * @author fanyu
 * @Date:2019年01月22日
 * @version:1.0
 */
public class CreateSameRegionByDiffType17334 extends S3TestBase {
    private boolean runSuccess = false;
    private String regionName = "region17334";
    private String bucketName = "bucket17334";
    private String objectName = "object17334";
    private AmazonS3 s3Client = null;
    private SequoiaS3 regionClient = null;

    @BeforeClass
    private void setUp() throws Exception {
        s3Client = CommLib.buildS3Client();
        regionClient = CommLib.regionClient();
        RegionUtils.clearRegion( regionClient, regionName );
        CreateRegionRequest request = new CreateRegionRequest( regionName );
        request.withDataCSShardingType( DataShardingType.YEAR )
                .withDataCLShardingType( DataShardingType.MONTH );
        regionClient.createRegion( request );
    }

    @Test
    private void test() throws Exception {
        CreateRegion cThread = new CreateRegion( DataShardingType.MONTH,
                DataShardingType.QUARTER );
        cThread.start( 10 );
        Assert.assertEquals( cThread.isSuccess(), true, cThread.getErrorMsg() );
        // get region
        GetRegionResult result = regionClient.getRegion( regionName );
        // check region sharding type
        Region region = result.getRegion();
        Assert.assertEquals( region.getDataCSShardingType(),
                DataShardingType.MONTH );
        Assert.assertEquals( region.getDataCLShardingType(),
                DataShardingType.QUARTER );
        // craete bucket for check
        s3Client.createBucket(
                new CreateBucketRequest( bucketName, regionName ) );
        // create object for check
        s3Client.putObject( bucketName, objectName,
                String.valueOf( UUID.randomUUID() ) );
        // get object for check
        S3Object s3Object = s3Client.getObject( bucketName, objectName );
        Assert.assertEquals( s3Object.getBucketName(), bucketName );
        Assert.assertEquals( s3Object.getKey(), objectName );
        Assert.assertEquals( s3Object.getObjectMetadata().getVersionId(),
                "null" );
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
            regionClient.shutdown();
        }

    }

    private class CreateRegion extends S3ThreadBase {
        private DataShardingType dataCSShardingType;
        private DataShardingType dataCLShardingType;

        public CreateRegion( DataShardingType dataCSShardingType,
                DataShardingType dataCLShardingType ) {
            this.dataCSShardingType = dataCSShardingType;
            this.dataCLShardingType = dataCLShardingType;
        }

        @Override
        public void exec() throws Exception {
            SequoiaS3 regionClient = CommLib.regionClient();
            CreateRegionRequest request = new CreateRegionRequest( regionName );
            request.withDataCSShardingType( this.dataCSShardingType )
                    .withDataCLShardingType( this.dataCLShardingType );
            regionClient.createRegion( request );
            regionClient.shutdown();
        }
    }
}
