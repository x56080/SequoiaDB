package com.sequoias3.region;

import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.RegionUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Description seqDB-17294: create Region and specify cs and cl. the metaCL is
 *              the same as metaHisCL
 * @author wuyan
 * @Date 2019.1.21
 * @version 1.00
 */
public class CreateRegion17294 extends S3TestBase {
    private boolean runSuccess = false;
    private String regionName = "region17294";
    private String[] csNames = { "metaCS17294", "dataCS17294" };
    private String[] metaclNames = { "metaCL17294" };
    private String[] dataclNames = { "dataCL17294" };

    @BeforeClass
    private void setUp() throws Exception {
        RegionUtils.createCSAndCL( csNames[ 0 ], metaclNames );
        RegionUtils.createCSAndCL( csNames[ 1 ], dataclNames );
        RegionUtils.clearRegion( regionName );
    }

    @Test
    public void testRegion() throws Exception {
        try {
            Region region = new Region();
            String metaLocation = csNames[ 0 ] + "." + metaclNames[ 0 ];
            String metaHisLocation = csNames[ 0 ] + "." + metaclNames[ 0 ];
            String dataLocation = csNames[ 1 ] + "." + dataclNames[ 0 ];
            region.withMetaLocation( metaLocation )
                    .withMetaHisLocation( metaHisLocation )
                    .withDataLocation( dataLocation ).withName( regionName );
            RegionUtils.putRegion( region );
            Assert.fail( "put region must be fail!" );
        } catch ( AmazonS3Exception e ) {
            // return 400:InvalidLocation
            Assert.assertEquals( e.getStatusCode(), 400 );
            Assert.assertEquals( e.getErrorCode(), "InvalidLocation" );
        }

        Assert.assertFalse( RegionUtils.headRegion( regionName ),
                "region should be not exist!" );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        if ( runSuccess ) {
            RegionUtils.dropCS( csNames );
        }
    }
}
