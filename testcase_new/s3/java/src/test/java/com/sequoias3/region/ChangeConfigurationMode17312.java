package com.sequoias3.region;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.sequoiadb.base.Sequoiadb;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.bean.Region;
import com.sequoias3.testcommon.s3utils.RegionUtils;

/**
 * test content: 创建和更新区域配置方式不同 testlink-case: seqDB-17312
 *
 * @author wangkexin
 * @Date 2019.01.23
 * @version 1.00
 */

public class ChangeConfigurationMode17312 extends S3TestBase {
    private String specifiedRegionName = "ModeOne17312";
    private String shardingTypeRegionName = "ModeTwo17312";
    private String metaCSName = "metaCS17312";
    private String dataCSName = "dataCS17312";
    private String[] metaClNames = { "metaCL17312", "metaHistoryCL17312" };
    private String[] dataClName = { "dataCL17312" };
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {
        RegionUtils.createCSAndCL( metaCSName, metaClNames );
        RegionUtils.createCSAndCL( dataCSName, dataClName );
        RegionUtils.clearRegion( specifiedRegionName );
        RegionUtils.clearRegion( shardingTypeRegionName );
    }

    @Test
    public void testCreateRegion() throws Exception {
        // test a : change the specified mode to ShardingType mode
        Region oldRegion_1 = new Region();
        oldRegion_1.withName( specifiedRegionName )
                .withMetaLocation( metaCSName + "." + metaClNames[ 0 ] )
                .withMetaHisLocation( metaCSName + "." + metaClNames[ 1 ] )
                .withDataLocation( dataCSName + "." + dataClName[ 0 ] );
        RegionUtils.putRegion( oldRegion_1 );

        Assert.assertTrue( RegionUtils.headRegion( specifiedRegionName ) );

        Region newRegion_1 = new Region();
        newRegion_1.withName( specifiedRegionName )
                .withDataCSShardingType( "year" )
                .withDataCLShardingType( "month" );

        try {
            RegionUtils.putRegion( newRegion_1 );
            Assert.fail(
                    "change the specified mode to ShardingType mode should fail" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "ConflictRegionType" );
        }

        // test b : change ShardingType mode to the specified mode
        Region oldRegion_2 = new Region();
        oldRegion_2.withName( shardingTypeRegionName );
        RegionUtils.putRegion( oldRegion_2 );

        Assert.assertTrue( RegionUtils.headRegion( shardingTypeRegionName ) );

        Region newRegion_2 = new Region();
        newRegion_2.withName( shardingTypeRegionName )
                .withMetaLocation( metaCSName + "." + metaClNames[ 0 ] )
                .withMetaHisLocation( metaCSName + "." + metaClNames[ 1 ] )
                .withDataLocation( dataCSName + "." + dataClName[ 0 ] );

        try {
            RegionUtils.putRegion( newRegion_2 );
            Assert.fail(
                    "change ShardingType mode to the specified mode should fail" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "ConflictRegionType" );
        }
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        if ( runSuccess ) {
            try ( Sequoiadb sdb = new Sequoiadb( S3TestBase.coordUrl, "",
                    "" ) ;) {
                sdb.dropCollectionSpace( metaCSName );
                sdb.dropCollectionSpace( dataCSName );
                RegionUtils.deleteRegion( specifiedRegionName );
                RegionUtils.deleteRegion( shardingTypeRegionName );
            }
        }
    }
}
