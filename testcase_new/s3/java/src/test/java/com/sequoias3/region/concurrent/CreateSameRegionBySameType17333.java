package com.sequoias3.region.concurrent;

import com.sequoias3.region.GetRegionResult;
import com.sequoias3.region.Region;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.RegionUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * @Description: seqDB-17333::并发更新相同区域（配置一致）
 * @author fanyu
 * @Date:2019年01月22日
 * @version:1.0
 */
public class CreateSameRegionBySameType17333 extends S3TestBase {
    private boolean runSuccess = false;
    private String regionName = "region17333";
    private String dataCSShardingType = "year";
    private String dataCLShardingType = "year";

    @BeforeClass
    private void setUp() throws Exception {
        RegionUtils.clearRegion( regionName );
    }

    @Test(threadPoolSize = 2, invocationCount = 2)
    private void test() throws Exception {
        // create region
        Region region = new Region()
                .withDataCSShardingType( this.dataCSShardingType )
                .withDataCLShardingType( this.dataCLShardingType )
                .withName( regionName );
        RegionUtils.putRegion( region );

        // get region
        GetRegionResult result = RegionUtils.getRegion( regionName );
        Assert.assertEquals( result.getBuckets().size(), 0,
                result.getBuckets().toString() );
        Region actRegion = result.getRegion();
        Assert.assertEquals( actRegion.getDataCSShardingType(), "year" );
        Assert.assertEquals( actRegion.getDataCLShardingType(), "year" );
        Assert.assertEquals( actRegion.getName(), regionName );
        Assert.assertEquals( actRegion.getDataLocation(), "" );
        Assert.assertEquals( actRegion.getMetaLocation(), "" );
        Assert.assertEquals( actRegion.getMetaHisLocation(), "" );
        Assert.assertEquals( actRegion.getMetaDomain(), "" );
        Assert.assertEquals( actRegion.getDataDomain(), "" );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        if ( runSuccess ) {
            RegionUtils.deleteRegion( regionName );
        }
    }
}
