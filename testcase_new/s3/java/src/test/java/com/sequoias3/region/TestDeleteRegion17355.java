package com.sequoias3.region;

import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.sequoiadb.base.Sequoiadb;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.RegionUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * test content: DeleteRegion接口参数校验 testlink-case: seqDB-17355
 *
 * @author wangkexin
 * @Date 2019.01.24
 * @version 1.00
 */

public class TestDeleteRegion17355 extends S3TestBase {
    private String regionName = "Beijing17355";
    private String metaCSName = "metaCS17355";
    private String dataCSName = "dataCS17355";
    private String[] metaClNames = { "metaCL17355", "metaHistoryCL17355" };
    private String[] dataClName = { "dataCL17355" };

    @BeforeClass
    private void setUp() throws Exception {
        RegionUtils.createCSAndCL( metaCSName, metaClNames );
        RegionUtils.createCSAndCL( dataCSName, dataClName );

        RegionUtils.clearRegion( regionName );
        Region region = new Region();
        region.withName( regionName )
                .withMetaLocation( metaCSName + "." + metaClNames[ 0 ] )
                .withMetaHisLocation( metaCSName + "." + metaClNames[ 1 ] )
                .withDataLocation( dataCSName + "." + dataClName[ 0 ] );
        RegionUtils.putRegion( region );
    }

    @Test
    public void testCreateRegion() throws Exception {
        // 合法值
        RegionUtils.deleteRegion( regionName );
        Assert.assertFalse( RegionUtils.headRegion( regionName ) );

        // 非法值 SEQUOIADBMAINSTREAM-4186
        try {
            RegionUtils.deleteRegion( "" );
            Assert.fail(
                    "put region with illegal region name '' should fail!" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "InvalidRegionName" );
        }

        try {
            RegionUtils.deleteRegion( new String() );
            Assert.fail(
                    "put region with illegal region name null should fail!" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "InvalidRegionName" );
        }
    }

    @AfterClass
    private void tearDown() throws Exception {
        try ( Sequoiadb sdb = new Sequoiadb( S3TestBase.coordUrl, "", "" ) ) {
            sdb.dropCollectionSpace( dataCSName );
            sdb.dropCollectionSpace( metaCSName );
        }
    }
}
