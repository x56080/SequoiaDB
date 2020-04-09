package com.sequoias3.region.concurrent;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.S3Object;
import com.sequoias3.testcommon.s3utils.bean.GetRegionResult;
import com.sequoias3.testcommon.s3utils.bean.Region;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.S3ThreadBase;
import com.sequoias3.testcommon.s3utils.RegionUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.HashMap;
import java.util.Map;
import java.util.UUID;

/**
 * @Description: seqDB-17332 :: 并发创建相同区域（配置不同）
 * @author fanyu
 * @Date:2019年01月22日
 * @version:1.0
 */
public class CreateSameRegionByDiffType17332 extends S3TestBase {
    private boolean runSuccess = false;
    private String regionName = "region17332";
    private String bucketName = "bucket17332";
    private String objectName = "object17332";
    private AmazonS3 s3Client = null;

    @BeforeClass
    private void setUp() throws Exception {
        s3Client = CommLib.buildS3Client();
        RegionUtils.clearRegion( regionName );
    }

    @Test
    private void test() throws Exception {
        Map< String, String > hashMap1 = new HashMap< String, String >();
        hashMap1.put( "dataCSShardingType", "month" );
        hashMap1.put( "dataCLShardingType", "quarter" );

        Map< String, String > hashMap2 = new HashMap< String, String >();
        hashMap2.put( "dataCSShardingType", "quarter" );
        hashMap2.put( "dataCLShardingType", "year" );

        CreateRegion cThread1 = new CreateRegion(
                hashMap1.get( "dataCSShardingType" ),
                hashMap1.get( "dataCLShardingType" ) );
        CreateRegion cThread2 = new CreateRegion(
                hashMap2.get( "dataCSShardingType" ),
                hashMap2.get( "dataCLShardingType" ) );
        cThread1.start( 20 );
        cThread2.start( 20 );
        Assert.assertEquals( cThread1.isSuccess(), true,
                cThread1.getErrorMsg() );
        Assert.assertEquals( cThread2.isSuccess(), true,
                cThread2.getErrorMsg() );

        // get region
        GetRegionResult region = RegionUtils.getRegion( regionName );
        Map< String, String > actMap = new HashMap< String, String >();
        actMap.put( "dataCSShardingType",
                region.getRegion().getDataCSShardingType() );
        actMap.put( "dataCLShardingType",
                region.getRegion().getDataCLShardingType() );
        // check region sharding type
        if ( !actMap.toString().equals( hashMap1.toString() )
                && !actMap.toString().equals( hashMap2.toString() ) ) {
            throw new Exception( "actMap = " + actMap.toString()
                    + ",hashMap1 = " + hashMap1.toString() + ",hashMap2 = "
                    + hashMap2.toString() );
        }

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
