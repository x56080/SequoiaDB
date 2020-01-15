package com.sequoias3.config;

import java.util.concurrent.atomic.AtomicInteger;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.bean.Region;
import com.sequoias3.testcommon.s3utils.RegionUtils;

/**
 * test content: 使用指定模式创建区域，带LobPageSize和RepliSize参数testlink-case: seqDB-18610
 *
 * @author wangkexin
 * @Date 2019.06.27
 * @version 1.00
 */
public class CreateRegion18610 extends S3TestBase {
    private AtomicInteger actSuccessTests = new AtomicInteger( 0 );
    private String regionName = "region18610";
    private AmazonS3 s3Client = null;
    private String[] csNames = { "metaCS18610", "dataCS18610" };
    private String[] metaclNames = { "metaCL18610", "metaHistroyCL18610" };
    private String[] dataclNames = { "dataCL18610" };

    @DataProvider(name = "lobPageSizeAndreplSizeProvider")
    public Object[][] generateAuthorization() {
        return new Object[][] { new Object[] { "4096", "" },
                new Object[] { "", "3" }, new Object[] { "8192", "1" } };
    }

    @BeforeClass
    private void setUp() throws Exception {
        RegionUtils.clearRegion( regionName );
        RegionUtils.createCSAndCL( csNames[ 0 ], metaclNames );
        RegionUtils.createCSAndCL( csNames[ 1 ], dataclNames );

        s3Client = CommLib.buildS3Client();
    }

    @Test(dataProvider = "lobPageSizeAndreplSizeProvider")
    public void testRegion( String dataLobPageSize, String dataReplSize )
            throws Exception {
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
            Assert.fail( "expected failure but actually succeeded." );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "ConflictRegionType" );
        }

        actSuccessTests.getAndIncrement();
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( actSuccessTests.get() == generateAuthorization().length ) {
                RegionUtils.dropCS( csNames );
            }
        } finally {
            s3Client.shutdown();
        }
    }
}
