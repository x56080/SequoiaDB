package com.sequoias3.region.concurrent;

import java.util.concurrent.atomic.AtomicInteger;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoias3.SequoiaS3;
import com.sequoias3.common.DataShardingType;
import com.sequoias3.model.CreateRegionRequest;
import com.sequoias3.model.GetRegionResult;
import com.sequoias3.model.Region;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.RegionUtils;

/**
 * @Description: seqDB-17333::并发更新相同区域（配置一致）
 * @author fanyu
 * @Date:2019年01月22日
 * @version:1.0
 */
public class CreateSameRegionBySameType17333 extends S3TestBase {
    private AtomicInteger actSuccessTests = new AtomicInteger( 0 );
    private String regionName = "region17333";
    private DataShardingType dataCSShardingType = DataShardingType.YEAR;
    private DataShardingType dataCLShardingType = DataShardingType.YEAR;
    private SequoiaS3 regionClient = null;

    @BeforeClass
    private void setUp() throws Exception {
        regionClient = CommLib.regionClient();
        RegionUtils.clearRegion( regionClient, regionName );
    }

    @Test(threadPoolSize = 2, invocationCount = 2)
    private void test() throws Exception {
        // create region
        SequoiaS3 regionClient1 = CommLib.regionClient();
        CreateRegionRequest request = new CreateRegionRequest( regionName );
        request.withDataCSShardingType( this.dataCSShardingType )
                .withDataCLShardingType( this.dataCLShardingType );
        regionClient1.createRegion( request );

        // get region
        GetRegionResult result = regionClient1.getRegion( regionName );
        Assert.assertEquals( result.getBuckets().size(), 0,
                result.getBuckets().toString() );
        Region actRegion = result.getRegion();
        Assert.assertEquals( actRegion.getDataCSShardingType(),
                DataShardingType.YEAR );
        Assert.assertEquals( actRegion.getDataCLShardingType(),
                DataShardingType.YEAR );
        Assert.assertEquals( actRegion.getName(), regionName );
        Assert.assertEquals( actRegion.getDataLocation(), null );
        Assert.assertEquals( actRegion.getMetaLocation(), null );
        Assert.assertEquals( actRegion.getMetaHisLocation(), null );
        Assert.assertEquals( actRegion.getMetaDomain(), null );
        Assert.assertEquals( actRegion.getDataDomain(), null );
        regionClient1.shutdown();
        actSuccessTests.getAndIncrement();
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( actSuccessTests.get() == 2 ) {
                regionClient.deleteRegion( regionName );
            }
        } finally {
            regionClient.shutdown();
        }

    }
}
