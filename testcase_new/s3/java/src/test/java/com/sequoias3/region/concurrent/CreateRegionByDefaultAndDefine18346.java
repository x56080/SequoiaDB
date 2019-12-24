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
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import java.util.UUID;

/**
 * @Description: seqDB-18346:并发创建相同区域（配置使用默认值和非默认值）
 * @author fanyu
 * @Date:2019年01月22日
 * @version:1.0
 */
public class CreateRegionByDefaultAndDefine18346 extends S3TestBase {
    String dataCSShardingType = "quarter";
    String dataCLShardingType = "month";
    private String regionName = "region18346";
    private String bucketName = "bucket18346";
    private String objectName = "object18346";
    private AmazonS3 s3Client = null;

    @BeforeClass
    private void setUp() throws Exception {
        s3Client = CommLib.buildS3Client();
        RegionUtils.clearRegion( regionName );
    }

    @DataProvider(name = "data-provier")
    private Object[][] dataProvier() {
        Region defineRegion = new Region()
                .withDataCSShardingType( dataCSShardingType )
                .withDataCLShardingType( dataCLShardingType )
                .withName( regionName );
        return new Object[][] {
                // dataCSShardingType and dataCLShardingType does not set
                { new Region().withName( regionName ), defineRegion },
                // dataCLShardingType does not set
                { new Region().withName( regionName ).withDataCSShardingType(
                        dataCSShardingType ), defineRegion },
                // dataCSShardingType does not set
                { new Region().withName( regionName ).withDataCLShardingType(
                        dataCLShardingType ), defineRegion } };
    }

    @Test(dataProvider = "data-provier")
    private void test( Region defaultRegion, Region defineRegion )
            throws Exception {
        CreateRegion cThread1 = new CreateRegion( defaultRegion );
        CreateRegion cThread2 = new CreateRegion( defineRegion );
        cThread1.start();
        cThread2.start();
        Assert.assertEquals( cThread1.isSuccess(), true,
                cThread1.getErrorMsg() );
        Assert.assertEquals( cThread2.isSuccess(), true,
                cThread2.getErrorMsg() );

        // get region
        GetRegionResult result = RegionUtils.getRegion( regionName );
        Assert.assertEquals( result.getRegion().getDataCSShardingType(),
                dataCSShardingType, result.toString() );
        Assert.assertEquals( result.getRegion().getDataCLShardingType(),
                dataCLShardingType, result.toString() );

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
        // clean
        CommLib.clearBucket( s3Client, bucketName );
        RegionUtils.deleteRegion( regionName );
    }

    @AfterClass
    private void tearDown() throws Exception {
        s3Client.shutdown();
    }

    private class CreateRegion extends S3ThreadBase {
        private Region region;

        public CreateRegion( Region region ) {
            this.region = region;
        }

        @Override
        public void exec() throws Exception {
            RegionUtils.putRegion( region );
        }
    }
}
