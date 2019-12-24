package com.sequoias3.region;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.sequoiadb.base.Sequoiadb;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.RegionUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * test content: 创建桶指定区域，获取区域信息 testlink-case: seqDB-17313
 *
 * @author wangkexin
 * @Date 2019.01.23
 * @version 1.00
 */

public class GetRegionMessage17313 extends S3TestBase {
    private String specifiedRegionName = "ModeOne17313";
    private String shardingTypeRegionName = "ModeTwo17313";
    private String metaCSName = "metaCS17313";
    private String dataCSName = "dataCS17313";
    private String specifiedBucket = "spbucket17313";
    private String shardingBucket = "shbucket17313";
    private String[] metaClNames = { "metaCL17313", "metaHistoryCL17313" };
    private String[] dataClName = { "dataCL17313" };
    private String dataDomain = "dataDomain17313";
    private String metaDomain = "metaDomain17313";
    private AmazonS3 s3Client = null;
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {
        s3Client = CommLib.buildS3Client();
        RegionUtils.dropCS( metaCSName );
        RegionUtils.dropCS( dataCSName );
        ;
        RegionUtils.dropDomain( dataDomain );
        RegionUtils.dropDomain( metaDomain );

        RegionUtils.createDomain( dataDomain );
        RegionUtils.createDomain( metaDomain );

        RegionUtils.createCSAndCL( metaCSName, metaClNames );
        RegionUtils.createCSAndCL( dataCSName, dataClName );

        RegionUtils.clearRegion( specifiedRegionName );
        RegionUtils.clearRegion( shardingTypeRegionName );
    }

    @Test
    public void testGetRegionMessage() throws Exception {
        String metaLocation = metaCSName + "." + metaClNames[ 0 ];
        String metaHisLocation = metaCSName + "." + metaClNames[ 1 ];
        String dataLocation = dataCSName + "." + dataClName[ 0 ];

        // test a : in specified mode,get region message
        Region specifiedModeRegion = new Region();
        specifiedModeRegion.withName( specifiedRegionName )
                .withMetaLocation( metaLocation )
                .withMetaHisLocation( metaHisLocation )
                .withDataLocation( dataLocation );
        RegionUtils.putRegion( specifiedModeRegion );

        Assert.assertTrue( RegionUtils.headRegion( specifiedRegionName ) );
        s3Client.createBucket( new CreateBucketRequest( specifiedBucket,
                specifiedRegionName.toLowerCase() ) );
        Assert.assertEquals( s3Client.getBucketLocation( specifiedBucket ),
                specifiedRegionName.toLowerCase() );

        GetRegionResult specifiedResult = RegionUtils
                .getRegion( specifiedRegionName );
        Region currRegion = specifiedResult.getRegion();
        Assert.assertEquals( currRegion.getName(),
                specifiedRegionName.toLowerCase() );
        Assert.assertEquals( currRegion.getMetaLocation(), metaLocation );
        Assert.assertEquals( currRegion.getMetaHisLocation(), metaHisLocation );
        Assert.assertEquals( currRegion.getDataLocation(), dataLocation );

        // test b : in ShardingType mode,get region message
        Region shardingTypeModeRegion = new Region();
        shardingTypeModeRegion.withName( shardingTypeRegionName )
                .withDataCSShardingType( "quarter" )
                .withDataCLShardingType( "month" ).withDataDomain( dataDomain )
                .withMetaDomain( metaDomain );
        RegionUtils.putRegion( shardingTypeModeRegion );

        Assert.assertTrue( RegionUtils.headRegion( shardingTypeRegionName ) );
        s3Client.createBucket( new CreateBucketRequest( shardingBucket,
                shardingTypeRegionName.toLowerCase() ) );
        Assert.assertEquals( s3Client.getBucketLocation( shardingBucket ),
                shardingTypeRegionName.toLowerCase() );

        GetRegionResult shardingTypeResult = RegionUtils
                .getRegion( shardingTypeRegionName );
        Region currRegion2 = shardingTypeResult.getRegion();
        Assert.assertEquals( currRegion2.getName(),
                shardingTypeRegionName.toLowerCase() );
        Assert.assertEquals( currRegion2.getDataCSShardingType(), "quarter" );
        Assert.assertEquals( currRegion2.getDataCLShardingType(), "month" );
        Assert.assertEquals( currRegion2.getDataDomain(), dataDomain );
        Assert.assertEquals( currRegion2.getMetaDomain(), metaDomain );

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        if ( runSuccess ) {
            try ( Sequoiadb sdb = new Sequoiadb( S3TestBase.coordUrl, "",
                    "" ) ) {
                s3Client.deleteBucket( specifiedBucket );
                s3Client.deleteBucket( shardingBucket );
                sdb.dropCollectionSpace( dataCSName );
                sdb.dropCollectionSpace( metaCSName );
                RegionUtils.deleteRegion( specifiedRegionName );
                RegionUtils.deleteRegion( shardingTypeRegionName );
                RegionUtils.dropDomain( metaDomain );
                RegionUtils.dropDomain( dataDomain );
            }
        }
    }
}
