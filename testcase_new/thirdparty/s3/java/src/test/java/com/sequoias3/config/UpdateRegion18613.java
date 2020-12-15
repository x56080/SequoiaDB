package com.sequoias3.config;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoias3.SequoiaS3;
import com.sequoias3.exception.SequoiaS3ServiceException;
import com.sequoias3.model.CreateRegionRequest;
import com.sequoias3.model.GetRegionResult;
import com.sequoias3.model.Region;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.RegionUtils;

/**
 * @Description seqDB-18613:更新指定模式区域配置DataLobPageSize和DataReplSize
 *
 * @author wangkexin
 * @Date 2019.06.27
 * @version 1.00
 */
public class UpdateRegion18613 extends S3TestBase {
    private boolean runSuccess = false;
    private String regionName = "region18613";
    private String[] csNames = { "metaCS18613", "dataCS18613" };
    private String[] metaclNames = { "metaCL18613", "metaHistroyCL18613" };
    private String[] dataclNames = { "dataCL18613" };
    private int dataLobPageSize = 4096;
    private int dataReplSize = 2;
    private SequoiaS3 regionClient = null;

    @BeforeClass
    private void setUp() throws Exception {
        RegionUtils.createCSAndCL( csNames[ 0 ], metaclNames );
        RegionUtils.createCSAndCL( csNames[ 1 ], dataclNames );
        regionClient = CommLib.regionClient();
        RegionUtils.clearRegion( regionClient, regionName );

        String metaLocation = csNames[ 0 ] + "." + metaclNames[ 0 ];
        String metaHisLocation = csNames[ 0 ] + "." + metaclNames[ 1 ];
        String dataLocation = csNames[ 1 ] + "." + dataclNames[ 0 ];
        CreateRegionRequest request = new CreateRegionRequest( regionName );
        request.withMetaLocation( metaLocation )
                .withDataLocation( dataLocation )
                .withMetaHisLocation( metaHisLocation );
        regionClient.createRegion( request );
    }

    @Test
    public void testRegion() throws Exception {
        // update region
        String metaLocation = csNames[ 0 ] + "." + metaclNames[ 0 ];
        String metaHisLocation = csNames[ 0 ] + "." + metaclNames[ 1 ];
        String dataLocation = csNames[ 1 ] + "." + dataclNames[ 0 ];
        CreateRegionRequest request = new CreateRegionRequest( regionName );
        request.withMetaLocation( metaLocation )
                .withDataLocation( dataLocation )
                .withMetaHisLocation( metaHisLocation )
                .withDataLobPageSize( dataLobPageSize )
                .withDataReplSize( dataReplSize );

        try {
            regionClient.createRegion( request );
            Assert.fail( "exp failed but found succeed." );
        } catch ( SequoiaS3ServiceException e ) {
            Assert.assertEquals( e.getErrorCode(), "ConflictRegionType" );
        }
        checkLobPageSizeAndReplSize();
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( runSuccess ) {
                regionClient.deleteRegion( regionName );
                RegionUtils.dropCS( csNames );

            }
        } finally {
            regionClient.shutdown();
        }
    }

    private void checkLobPageSizeAndReplSize() throws Exception {
        GetRegionResult result = regionClient.getRegion( regionName );
        Region region = result.getRegion();
        Assert.assertEquals( region.getDataLobPageSize(), null );
        Assert.assertEquals( region.getDataReplSize(), null );
    }
}
