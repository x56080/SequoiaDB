package com.sequoias3.region;

import java.util.concurrent.atomic.AtomicInteger;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import com.sequoias3.SequoiaS3;
import com.sequoias3.common.DataShardingType;
import com.sequoias3.exception.SequoiaS3ServiceException;
import com.sequoias3.model.CreateRegionRequest;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.RegionUtils;

/**
 * @Description: seqDB-17301 :: 创建区域配置不存在的domain
 * @author fanyu
 * @Date:2019年01月22日
 * @version:1.0
 */
public class CreateRegion17301 extends S3TestBase {
    private AtomicInteger actSuccessTests = new AtomicInteger( 0 );
    private String[] regionNames = new String[] { "region17301a",
            "region17301b", "region17301c" };
    private String domainName1 = "doesNotExist17301";
    private String domainName2 = "Exist17301";
    private SequoiaS3 regionClient = null;

    @BeforeClass
    private void setUp() throws Exception {
        regionClient = CommLib.regionClient();
        for ( String regionName : regionNames ) {
            RegionUtils.clearRegion( regionClient, regionName );
        }
    }

    @DataProvider(name = "range-provider")
    private Object[][] rangeData() {
        RegionUtils.createDomain( domainName2 );
        return new Object[][] {
                // regionName dataDomain metaDomain
                { regionNames[ 0 ], domainName1, domainName2 },
                { regionNames[ 1 ], domainName2, domainName1 },
                { regionNames[ 2 ], domainName1, domainName1 } };
    }

    @Test(dataProvider = "range-provider")
    private void test( String regionName, String dataDomain, String metaDomain )
            throws Exception {
        // create region
        CreateRegionRequest request = new CreateRegionRequest( regionName );
        request.withDataCSShardingType( DataShardingType.YEAR )
                .withDataCLShardingType( DataShardingType.YEAR )
                .withDataDomain( dataDomain ).withMetaDomain( metaDomain );
        try {
            regionClient.createRegion( request );
            Assert.fail( "exp fail but act success,regionName = " + ""
                    + regionName + ",datadomain = " + dataDomain
                    + ",metaDomain = " + metaDomain );
        } catch ( SequoiaS3ServiceException e ) {
            if ( e.getStatusCode() != 400
                    && !e.getErrorCode().contains( "InvalidDomain" ) ) {
                throw e;
            }
        }
        // check region that was not created successfully
        Assert.assertFalse( regionClient.headRegion( regionName ) );
        actSuccessTests.getAndIncrement();
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( actSuccessTests.get() == rangeData().length ) {
                RegionUtils.dropDomain( domainName2 );
            }
        } finally {
            regionClient.shutdown();
        }
    }
}
