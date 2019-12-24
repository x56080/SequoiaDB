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
 * @Description: seqDB-17307 :: 更新区域配置domain，指定domain不存在
 * @author fanyu
 * @Date:2019年01月22日
 * @version:1.0
 */
public class UpdateRegion17307 extends S3TestBase {
    private String[] regionNames = new String[] { "region17307a",
            "region17307b" };
    private String existDomain = "domain17307";
    private String updateDomain = "doesNotExistdomain17307";
    private AtomicInteger actSuccessTests = new AtomicInteger( 0 );

    @BeforeClass
    private void setUp() throws Exception {
        for ( String regionName : regionNames ) {
            RegionUtils.clearRegion( regionName );
        }
    }

    @DataProvider(name = "range-provider")
    private Object[][] rangeData() {
        RegionUtils.createDomain( existDomain );
        return new Object[][] {
                // regionName dataDomain metaDomain upDataDomain upMeatDomain
                { regionNames[ 0 ], existDomain, existDomain, updateDomain,
                        existDomain },
                { regionNames[ 1 ], existDomain, existDomain, existDomain,
                        updateDomain }, };
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
        try {
            RegionUtils.putRegion( region );
            Assert.fail( "exp failed but act success,region = " + region
                    .toString() );
        } catch ( AmazonS3Exception e ) {
            if ( e.getStatusCode() != 409 && !e.getErrorCode()
                    .contains( "ConflictDomain" ) ) {
                throw e;
            }
        }
        GetRegionResult result = RegionUtils.getRegion( regionName );
        Assert.assertEquals( result.getRegion().getDataDomain(), dataDomain );
        Assert.assertEquals( result.getRegion().getMetaDomain(), metaDomain );
        actSuccessTests.getAndIncrement();
    }

    @AfterClass
    private void tearDown() throws Exception {
        if ( actSuccessTests.get() == rangeData().length ) {
            for ( String regionName : regionNames ) {
                RegionUtils.deleteRegion( regionName );
            }
            RegionUtils.dropDomain( existDomain );
        }
    }
}
