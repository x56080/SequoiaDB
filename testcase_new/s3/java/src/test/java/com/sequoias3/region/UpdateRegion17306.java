package com.sequoias3.region;

import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.RegionUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import java.util.concurrent.atomic.AtomicInteger;

/**
 * @author fanyu
 * @Description: seqDB-17306 :: 更新区域配置domain
 * @Date:2019年01月22日
 * @version:1.0
 */
public class UpdateRegion17306 extends S3TestBase {
    private String[] domainNames = { "domain17306A", "domain17306B" };
    private String[] regionNames = { "region17306a", "region17306b",
            "region17306c" };
    private AtomicInteger actSuccessTests = new AtomicInteger( 0 );

    @BeforeClass
    private void setUp() throws Exception {
        for ( String regionName : regionNames ) {
            RegionUtils.clearRegion( regionName );
        }
    }

    @DataProvider(name = "range-provider")
    private Object[][] rangeData() {
        for ( String domainName : domainNames ) {
            RegionUtils.createDomain( domainName );
        }
        return new Object[][] {
                // regionName dataDomain metaDomain updateDataDomain
                // updateMeatDomain
                { regionNames[ 0 ], domainNames[ 0 ], domainNames[ 1 ],
                        domainNames[ 1 ], domainNames[ 1 ] },
                { regionNames[ 1 ], domainNames[ 0 ], domainNames[ 1 ],
                        domainNames[ 0 ], domainNames[ 0 ] },
                { regionNames[ 2 ], domainNames[ 0 ], domainNames[ 1 ],
                        domainNames[ 0 ], domainNames[ 1 ] }, };
    }

    @Test(dataProvider = "range-provider")
    private void test( String regionName, String dataDomain, String metaDomain,
            String upDataDomain, String upMeatDomain ) throws Exception {
        // create region
        Region region = new Region();
        region.withDataCSShardingType( "year" ).withDataCLShardingType( "year" )
                .withDataDomain( dataDomain ).withMetaDomain( metaDomain )
                .withName( regionName );
        RegionUtils.putRegion( region );

        region.withDataCSShardingType( "year" ).withDataCLShardingType( "year" )
                .withDataDomain( upDataDomain ).withMetaDomain( upMeatDomain )
                .withName( regionName );
        // Updated domain is not same as before when the index of regionName is
        // not equal to 2
        if ( !regionName.equals( regionNames[ 2 ] ) ) {
            try {
                // update region
                RegionUtils.putRegion( region );
                Assert.fail( "exp failed but act success,region = " + region
                        .toString() );
            } catch ( AmazonS3Exception e ) {
                if ( e.getStatusCode() != 409 && !e.getErrorCode()
                        .contains( "ConflictDomain" ) ) {
                    throw e;
                }
            }
        } else {
            // update region
            RegionUtils.putRegion( region );
            GetRegionResult result = RegionUtils.getRegion( regionName );
            Assert.assertEquals( result.getRegion().getDataDomain(),
                    upDataDomain );
            Assert.assertEquals( result.getRegion().getMetaDomain(),
                    upMeatDomain );
        }
        actSuccessTests.getAndIncrement();
    }

    @AfterClass
    private void tearDown() throws Exception {
        if ( actSuccessTests.get() == rangeData().length ) {
            for ( String regionName : regionNames ) {
                RegionUtils.deleteRegion( regionName );
            }
            for ( String domainName : domainNames ) {
                RegionUtils.dropDomain( domainName );
            }
        }
    }
}
