package com.sequoias3.region.concurrent;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.S3Object;
import com.sequoias3.region.GetRegionResult;
import com.sequoias3.region.Region;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.S3ThreadBase;
import com.sequoias3.testcommon.s3utils.RegionUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.UUID;

/**
 * @Description: seqDB-17334 :: 并发更新相同区域（配置不同）
 * @author fanyu
 * @Date:2019年01月22日
 * @version:1.0
 */
public class CreateSameRegionByDiffType17334 extends S3TestBase {
    private boolean runSuccess = false;
    private String regionName = "region17332";
    private String bucketName = "bucket17332";
    private String objectName = "object17332";
    private AmazonS3 s3Client = null;

    @BeforeClass
    private void setUp() throws Exception {
        s3Client = CommLib.buildS3Client();
        RegionUtils.clearRegion( regionName );
        Region region = new Region().withDataCSShardingType( "year" )
                .withDataCLShardingType( "month" ).withName( regionName );
        RegionUtils.putRegion( region );
    }

    @Test
    private void test() throws Exception {
        CreateRegion cThread = new CreateRegion( "month", "quarter" );
        cThread.start( 10 );
        Assert.assertEquals( cThread.isSuccess(), true, cThread.getErrorMsg() );
        // get region
        GetRegionResult result = RegionUtils.getRegion( regionName );
        // check region sharding type
        Region region = result.getRegion();
        Assert.assertEquals( region.getDataCSShardingType(), "month" );
        Assert.assertEquals( region.getDataCLShardingType(), "quarter" );
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
        if ( runSuccess ) {
            CommLib.clearBucket( s3Client, bucketName );
            RegionUtils.deleteRegion( regionName );
        }
    }

    private class CreateRegion extends S3ThreadBase {
        private String dataCSShardingType;
        private String dataCLShardingType;

        public CreateRegion( String dataCSShardingType,
                String dataCLShardingType ) {
            this.dataCSShardingType = dataCSShardingType;
            this.dataCLShardingType = dataCLShardingType;
        }

        @Override
        public void exec() throws Exception {
            Region region = new Region()
                    .withDataCSShardingType( this.dataCSShardingType )
                    .withDataCLShardingType( this.dataCLShardingType )
                    .withName( regionName );
            RegionUtils.putRegion( region );
        }
    }
}
