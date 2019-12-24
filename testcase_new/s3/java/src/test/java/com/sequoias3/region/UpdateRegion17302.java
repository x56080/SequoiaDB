package com.sequoias3.region;

import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.RegionUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Description: seqDB-17302 :: 更新区域使用指定模式，指定配置不一致
 * @author fanyu
 * @Date:2019年01月22日
 * @version:1.0
 */
public class UpdateRegion17302 extends S3TestBase {
    private String regionName = "region17302";
    private String csName = "cs17302";
    private String[] clNames = { "dataCL17302A", "metaCL17302A",
            "metaHisCL17302A", "dataCL17302B", "metaCL17302B",
            "metaHisCL17302B" };
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {
        RegionUtils.createCSAndCL( csName, clNames );
        RegionUtils.clearRegion( regionName );
    }

    @Test
    private void test() throws Exception {
        // create region
        Region region = new Region();
        region.withDataLocation( csName + "." + clNames[ 0 ] )
                .withMetaLocation( csName + "." + clNames[ 1 ] )
                .withMetaHisLocation( csName + "." + clNames[ 2 ] )
                .withName( regionName );
        RegionUtils.putRegion( region );

        // create region again by different Meta and Data location
        try {
            Region region1 = new Region();
            region1.withDataCSShardingType( "year" )
                    .withDataCLShardingType( "year" )
                    .withDataLocation( csName + "." + clNames[ 3 ] )
                    .withMetaLocation( csName + "." + clNames[ 4 ] )
                    .withMetaHisLocation( csName + "." + clNames[ 5 ] )
                    .withName( regionName );
            RegionUtils.putRegion( region1 );
            Assert.fail( "put region must be failed, region = " + region
                    .toString() );
        } catch ( AmazonS3Exception e ) {
            if ( e.getStatusCode() != 409 && !e.getErrorCode()
                    .contains( "ConflictRegionType" ) ) {
                throw e;
            }
        }

        // check region info
        GetRegionResult regionResult = RegionUtils.getRegion( regionName );
        checkGetRegionResult( regionResult, region );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        if ( runSuccess ) {
            RegionUtils.dropCS( new String[] { csName } );
            RegionUtils.deleteRegion( regionName );
        }
    }

    private void checkGetRegionResult( GetRegionResult result,
            Region expRegion ) {
        Region actRegion = result.getRegion();
        Assert.assertEquals( actRegion.getDataLocation(),
                expRegion.getDataLocation() );
        Assert.assertEquals( actRegion.getMetaLocation(),
                expRegion.getMetaLocation() );
        Assert.assertEquals( actRegion.getMetaHisLocation(),
                expRegion.getMetaHisLocation() );
        Assert.assertEquals( result.getBuckets().size(), 0 );
    }
}
