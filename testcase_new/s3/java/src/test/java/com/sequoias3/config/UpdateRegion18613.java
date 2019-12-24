package com.sequoias3.config;

import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.sequoias3.region.GetRegionResult;
import com.sequoias3.region.Region;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.RegionUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * test content: 更新指定模式区域配置DataLobPageSize和DataReplSize testlink-case:
 * seqDB-18613
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
    private String dataLobPageSize = "4096";
    private String dataReplSize = "2";

    @BeforeClass
    private void setUp() throws Exception {
        RegionUtils.createCSAndCL( csNames[ 0 ], metaclNames );
        RegionUtils.createCSAndCL( csNames[ 1 ], dataclNames );
        RegionUtils.clearRegion( regionName );

        Region region = new Region();
        String metaLocation = csNames[ 0 ] + "." + metaclNames[ 0 ];
        String metaHisLocation = csNames[ 0 ] + "." + metaclNames[ 1 ];
        String dataLocation = csNames[ 1 ] + "." + dataclNames[ 0 ];
        region.withMetaLocation( metaLocation ).withDataLocation( dataLocation )
                .withMetaHisLocation( metaHisLocation ).withName( regionName );
        RegionUtils.putRegion( region );
    }

    @Test
    public void testRegion() throws Exception {
        // update region
        Region region = new Region();
        String metaLocation = csNames[ 0 ] + "." + metaclNames[ 0 ];
        String metaHisLocation = csNames[ 0 ] + "." + metaclNames[ 1 ];
        String dataLocation = csNames[ 1 ] + "." + dataclNames[ 0 ];
        region.withMetaLocation( metaLocation ).withDataLocation( dataLocation )
                .withMetaHisLocation( metaHisLocation ).withName( regionName )
                .withDataLobPageSize( dataLobPageSize )
                .withDataReplSize( dataReplSize );

        try {
            RegionUtils.putRegion( region );
            Assert.fail( "exp failed but found succeed." );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "ConflictRegionType" );
        }
        checkLobPageSizeAndReplSize();
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        if ( runSuccess ) {
            RegionUtils.deleteRegion( regionName );
            RegionUtils.dropCS( csNames );
        }
    }

    private void checkLobPageSizeAndReplSize() throws Exception {
        GetRegionResult result = RegionUtils.getRegion( regionName );
        Region region = result.getRegion();
        Assert.assertEquals( region.getDataLobPageSize(), "" );
        Assert.assertEquals( region.getDataReplSize(), "" );
    }
}
