package com.sequoias3.region;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.Sequoiadb;
import com.sequoias3.SequoiaS3;
import com.sequoias3.exception.SequoiaS3ServiceException;
import com.sequoias3.model.CreateRegionRequest;
import com.sequoias3.model.GetRegionResult;
import com.sequoias3.model.Region;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.RegionUtils;

/**
 * @Description seqDB-17354:GetRegion接口参数校验
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
    private SequoiaS3 regionClient = null;

    @BeforeClass
    private void setUp() throws Exception {
        RegionUtils.createCSAndCL( metaCSName, metaClNames );
        RegionUtils.createCSAndCL( dataCSName, dataClName );

        regionClient = CommLib.regionClient();
        RegionUtils.clearRegion( regionClient, regionName );

        CreateRegionRequest request = new CreateRegionRequest( regionName );
        request.withMetaLocation( metaCSName + "." + metaClNames[ 0 ] )
                .withMetaHisLocation( metaCSName + "." + metaClNames[ 1 ] )
                .withDataLocation( dataCSName + "." + dataClName[ 0 ] );
        regionClient.createRegion( request );
    }

    @Test
    public void testCreateRegion() throws Exception {
        // 合法值
        GetRegionResult result = regionClient.getRegion( regionName );
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
            regionClient.getRegion( "" );
            Assert.fail( "get region with '' region name should fail" );
        } catch ( SequoiaS3ServiceException e ) {
            Assert.assertEquals( e.getErrorCode(), "NoSuchRegion" );
        }

        try {
            regionClient.getRegion( null );
            Assert.fail( "get region with null region name should fail" );
        } catch ( IllegalArgumentException e ) {
            Assert.assertEquals( e.getMessage(), "Region name cannot be null" );
        }

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        if ( runSuccess ) {
            try ( Sequoiadb sdb = new Sequoiadb( S3TestBase.coordUrl, "",
                    "" )) {
                regionClient.deleteRegion( regionName );
                sdb.dropCollectionSpace( dataCSName );
                sdb.dropCollectionSpace( metaCSName );
            } finally {
                regionClient.shutdown();
            }
        }
    }
}
