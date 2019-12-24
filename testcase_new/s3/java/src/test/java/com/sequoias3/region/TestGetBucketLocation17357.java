package com.sequoias3.region;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.sequoiadb.base.Sequoiadb;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.RegionUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

/**
 * test content: GetBucketLocation接口参数校验 testlink-case: seqDB-17357
 *
 * @author wangkexin
 * @Date 2019.01.24
 * @version 1.00
 */

public class TestGetBucketLocation17357 extends S3TestBase {
    private String regionName = "Beijing17357";
    private String metaCSName = "metaCS17357";
    private String dataCSName = "dataCS17357";
    private String[] metaClNames = { "metaCL17357", "metaHistoryCL17357" };
    private String[] dataClName = { "dataCL17357" };
    private String bucketName = "bucket17357";
    private AmazonS3 s3Client = null;
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {
        RegionUtils.createCSAndCL( metaCSName, metaClNames );
        RegionUtils.createCSAndCL( dataCSName, dataClName );

        RegionUtils.clearRegion( regionName );
        Region region = new Region();
        region.withName( regionName )
                .withMetaLocation( metaCSName + "." + metaClNames[ 0 ] )
                .withMetaHisLocation( metaCSName + "." + metaClNames[ 1 ] )
                .withDataLocation( dataCSName + "." + dataClName[ 0 ] );
        RegionUtils.putRegion( region );

        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( new CreateBucketRequest( bucketName,
                regionName.toLowerCase() ) );
    }

    @Test
    public void testCreateRegion() throws Exception {
        // 合法值
        Assert.assertEquals( s3Client.getBucketLocation( bucketName ),
                regionName.toLowerCase() );

        // 非法值
        Assert.assertEquals( s3Client.getBucketLocation( "" ), "US" );

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( runSuccess ) {
                try ( Sequoiadb sdb = new Sequoiadb( S3TestBase.coordUrl, "",
                        "" ) ) {
                    CommLib.clearBucket( s3Client, bucketName );
                    RegionUtils.deleteRegion( regionName );
                    sdb.dropCollectionSpace( dataCSName );
                    sdb.dropCollectionSpace( metaCSName );
                }
            }
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }
}
