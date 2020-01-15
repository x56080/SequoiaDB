package com.sequoias3.region;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.RegionUtils;
import com.sequoias3.testcommon.s3utils.bean.Region;

/**
 * @Description seqDB-17292: create Region and specify incomplete configuartion
 *              information, eg:only specify metaLocation and dataLocation.
 * @author wuyan
 * @Date 2019.1.18
 * @version 1.00
 */
public class CreateRegion17292 extends S3TestBase {
    private boolean runSuccess = false;
    private String regionName = "region17292";
    private String[] csNames = { "metaCS17292", "dataCS17292" };
    private String[] dataclNames = { "metaCL17292", "metaHistroyCL17292" };
    private String[] metaclNames = { "dataCL17292" };

    @BeforeClass
    private void setUp() throws Exception {
        RegionUtils.createCSAndCL( csNames[ 0 ], dataclNames );
        RegionUtils.createCSAndCL( csNames[ 1 ], metaclNames );
        RegionUtils.clearRegion( regionName );
    }

    @Test
    public void testRegion() throws Exception {
        try {
            Region region = new Region();
            String metaLocation = "metaCS17291.metaCL17291";
            String dataLocation = "dataCS17291.dataCL17291";
            region.withMetaLocation( metaLocation )
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
