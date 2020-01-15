package com.sequoias3.region;

import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.sequoiadb.base.Sequoiadb;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.bean.GetRegionResult;
import com.sequoias3.testcommon.s3utils.bean.Region;
import com.sequoias3.testcommon.s3utils.RegionUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * test content: GetRegion接口参数校验 testlink-case: seqDB-17354
 *
 * @author wangkexin
 * @Date 2019.01.24
 * @version 1.00
 */

public class TestGetRegion17354 extends S3TestBase {
    private String regionName = "beijing17354";
    private String metaCSName = "metaCS17354";
    private String dataCSName = "dataCS17354";
    private String[] metaClNames = { "metaCL17354", "metaHistoryCL17354" };
    private String[] dataClName = { "dataCL17354" };
    private boolean runSuccess = false;

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
        GetRegionResult result = RegionUtils.getRegion( regionName );
        Region region = result.getRegion();
        Assert.assertEquals( region.getName(), regionName );
        Assert.assertEquals( region.getMetaLocation(),
                metaCSName + "." + metaClNames[ 0 ] );
        Assert.assertEquals( region.getMetaHisLocation(),
                metaCSName + "." + metaClNames[ 1 ] );
        Assert.assertEquals( region.getDataLocation(),
                dataCSName + "." + dataClName[ 0 ] );

        // 非法值
        try {
            RegionUtils.getRegion( "" );
            Assert.fail( "get region with '' region name should fail" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getStatusCode(), 404 );
        }

        try {
            RegionUtils.getRegion( new String() );
            Assert.fail(
                    "get region with new String() region name should fail" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getStatusCode(), 404 );
        }

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        if ( runSuccess ) {
            try ( Sequoiadb sdb = new Sequoiadb( S3TestBase.coordUrl, "",
                    "" ) ) {
                RegionUtils.deleteRegion( regionName );
                sdb.dropCollectionSpace( dataCSName );
                sdb.dropCollectionSpace( metaCSName );
            }
        }
    }
}
