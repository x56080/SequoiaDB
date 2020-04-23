package com.sequoias3.region;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.S3Object;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.RegionUtils;
import com.sequoias3.testcommon.s3utils.bean.GetRegionResult;
import com.sequoias3.testcommon.s3utils.bean.Region;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import java.util.UUID;

/**
 * @Description: seqDB-22092:自动创建模式配置DataLobPageSize和DataReplSize
 *               ，创建/列取/获取/更新/删除区域
 * @author fanyu
 * @Date:2019年04月21日
 * @version:1.0
 */
public class CrudRegion22092 extends S3TestBase {
    String dataCSShardingType = "quarter";
    String dataCLShardingType = "month";
    private String regionName = "region22092";
    private String bucketName = "bucket22092";
    private String objectName = "object22092";
    private AmazonS3 s3Client = null;

    @BeforeClass
    private void setUp() throws Exception {
        s3Client = CommLib.buildS3Client();
        RegionUtils.clearRegion( regionName );
    }

    @DataProvider(name = "data-provier")
    private Object[][] dataProvier() {
        return new Object[][] {
                // DataLobPageSize DataReplSize
                { "0", "-1" }, { "4096", "0" }, { "8192", "1" },
                { "16384", "2" }, { "32768", "3" }, { "65536", "4" },
                { "131072", "5" }, { "262144", "6" }, { "524288", "7" } };
    }

    @Test(dataProvider = "data-provier")
    private void test( String dataLobPageSize, String replSize )
            throws Exception {
        Region region = new Region()
                .withDataCSShardingType( dataCSShardingType )
                .withDataCLShardingType( dataCLShardingType )
                .withDataLobPageSize( dataLobPageSize )
                .withDataReplSize( replSize ).withName( regionName );

        // put region
        RegionUtils.putRegion( region );

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

        // get region
        GetRegionResult result1 = RegionUtils.getRegion( regionName );
        Assert.assertEquals( result1.getRegion().getDataLobPageSize(),
                dataLobPageSize, result1.toString() );
        Assert.assertEquals( result1.getRegion().getDataReplSize(), replSize,
                result1.toString() );

        // update region
        // the same as before
        Region updateRegion1 = new Region()
                .withDataCSShardingType( dataCSShardingType )
                .withDataCLShardingType( dataCLShardingType )
                .withDataLobPageSize( dataLobPageSize )
                .withDataReplSize( replSize ).withName( regionName );
        RegionUtils.putRegion( updateRegion1 );

        // update failed
        Region updateRegion2 = new Region()
                .withDataCSShardingType( dataCSShardingType )
                .withDataCLShardingType( dataCLShardingType )
                .withDataLobPageSize( "1024" ).withDataReplSize( "8" )
                .withName( regionName );
        try {
            RegionUtils.putRegion( updateRegion2 );
            Assert.fail( "exp failed but act success!!!" );
        } catch ( AmazonS3Exception e ) {
            if ( e.getStatusCode() != 409 ) {
                throw e;
            }
        }
        // check
        GetRegionResult result2 = RegionUtils.getRegion( regionName );
        Assert.assertEquals( result2.getRegion().getDataLobPageSize(),
                dataLobPageSize, result2.toString() );
        Assert.assertEquals( result2.getRegion().getDataReplSize(), replSize,
                result2.toString() );

        // clean
        CommLib.clearBucket( s3Client, bucketName );
        RegionUtils.deleteRegion( regionName );
        Assert.assertFalse( RegionUtils.headRegion( regionName ) );
    }

    @AfterClass
    private void tearDown() {
        if ( s3Client != null ) {
            s3Client.shutdown();
        }
    }
}
