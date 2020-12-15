package com.sequoias3.config;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.amazonaws.services.s3.AmazonS3;
import com.sequoias3.SequoiaS3;
import com.sequoias3.exception.SequoiaS3ClientException;
import com.sequoias3.exception.SequoiaS3ServiceException;
import com.sequoias3.model.CreateRegionRequest;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.RegionUtils;

/**
 * @Description: 使用指定模式创建区域，带LobPageSize和RepliSize参数testlink-case: seqDB-18610
 *
 * @author wangkexin
 * @Date 2019.06.27
 * @version 1.00
 */
public class CreateRegion18610 extends S3TestBase {
    private boolean runSuccess = false;
    private String regionName = "region18610";
    private AmazonS3 s3Client = null;
    private String[] csNames = { "metaCS18610", "dataCS18610" };
    private String[] metaclNames = { "metaCL18610", "metaHistroyCL18610" };
    private String[] dataclNames = { "dataCL18610" };
    private SequoiaS3 regionClient = null;

    @BeforeClass
    private void setUp() throws Exception {
        RegionUtils.createCSAndCL( csNames[ 0 ], metaclNames );
        RegionUtils.createCSAndCL( csNames[ 1 ], dataclNames );
        s3Client = CommLib.buildS3Client();
        regionClient = CommLib.regionClient();
        regionClient.headRegion( regionName );
        RegionUtils.clearRegion( regionClient, regionName );
    }

    @Test
    public void testRegion() throws SequoiaS3ClientException {
        String metaLocation = csNames[ 0 ] + "." + metaclNames[ 0 ];
        String metaHisLocation = csNames[ 0 ] + "." + metaclNames[ 1 ];
        String dataLocation = csNames[ 1 ] + "." + dataclNames[ 0 ];

        // test a:只配置DataLobPageSize
        CreateRegionRequest requesta = new CreateRegionRequest( regionName );
        requesta.withMetaLocation( metaLocation )
                .withDataLocation( dataLocation )
                .withMetaHisLocation( metaHisLocation )
                .withDataLobPageSize( 4096 );
        try {
            regionClient.createRegion( requesta );
            Assert.fail( "expected failure but actually succeeded." );
        } catch ( SequoiaS3ServiceException e ) {
            Assert.assertEquals( e.getErrorCode(), "ConflictRegionType" );
        }

        // test b:只配置DataReplSize
        CreateRegionRequest requestb = new CreateRegionRequest( regionName );
        requestb.withMetaLocation( metaLocation )
                .withDataLocation( dataLocation )
                .withMetaHisLocation( metaHisLocation ).withDataReplSize( 3 );
        try {
            regionClient.createRegion( requestb );
            Assert.fail( "expected failure but actually succeeded." );
        } catch ( SequoiaS3ServiceException e ) {
            Assert.assertEquals( e.getErrorCode(), "ConflictRegionType" );
        }

        // test c:同时配置DataLobPageSize和DataReplSize
        CreateRegionRequest requestc = new CreateRegionRequest( regionName );
        requestc.withMetaLocation( metaLocation )
                .withDataLocation( dataLocation )
                .withMetaHisLocation( metaHisLocation )
                .withDataLobPageSize( 4096 ).withDataReplSize( 3 );
        try {
            regionClient.createRegion( requestc );
            Assert.fail( "expected failure but actually succeeded." );
        } catch ( SequoiaS3ServiceException e ) {
            Assert.assertEquals( e.getErrorCode(), "ConflictRegionType" );
        }
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( runSuccess ) {
                RegionUtils.dropCS( csNames );
            }
        } finally {
            regionClient.shutdown();
            s3Client.shutdown();
        }
    }
}
