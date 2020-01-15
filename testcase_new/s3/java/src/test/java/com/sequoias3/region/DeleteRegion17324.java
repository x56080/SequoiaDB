package com.sequoias3.region;

import java.util.Date;
import java.util.UUID;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.bean.Region;
import com.sequoias3.testcommon.s3utils.RegionUtils;

/**
 * @Description: seqDB-17324:删除空区域
 * @author fanyu
 * @Date:2019年01月22日
 * @version:1.0
 */
public class DeleteRegion17324 extends S3TestBase {
    private AmazonS3 s3Client = null;
    private String bucketName = "bucket17324";
    private String objectName = "object17324";
    private String regionName1 = "region17324a";
    private String regionName2 = "region17324b";
    private String csName = "cs17324";
    private String[] clNames = { "dataCL17302A", "metaCL17302A",
            "metaHisCL17302A" };
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        RegionUtils.createCSAndCL( csName, clNames );
        RegionUtils.clearRegion( regionName1 );
        RegionUtils.clearRegion( regionName2 );
    }

    @Test
    private void testStatic() throws Exception {
        // create region
        Region region = new Region();
        region.withDataLocation( csName + "." + clNames[ 0 ] )
                .withMetaLocation( csName + "." + clNames[ 1 ] )
                .withMetaHisLocation( csName + "." + clNames[ 2 ] )
                .withName( regionName1 );
        RegionUtils.putRegion( region );

        // delete region
        RegionUtils.deleteRegion( regionName1 );

        // head region to make sure the region:regionName1 has been deleted
        Assert.assertFalse( RegionUtils.headRegion( regionName1 ),
                region.toString() );

        // check cs.cl has not been deleted
        for ( String clName : clNames ) {
            Assert.assertTrue( RegionUtils.clInCS( csName, clName ),
                    "csName = " + csName + ",clName = " + clName );
        }
        runSuccess = true;
    }

    @Test
    private void testDynamic() throws Exception {
        // create region
        Region region = new Region();
        region.withDataCSShardingType( "year" ).withDataCLShardingType( "year" )
                .withName( regionName2 );
        RegionUtils.putRegion( region );

        // create bucket and object to generate table
        s3Client.createBucket(
                new CreateBucketRequest( bucketName, regionName2 ) );
        s3Client.putObject( bucketName, objectName,
                String.valueOf( UUID.randomUUID() ) );
        CommLib.clearBucket( s3Client, bucketName );
        // delete region
        RegionUtils.deleteRegion( regionName2 );

        // head region to make sure the region:regionName1 has been deleted
        Assert.assertFalse( RegionUtils.headRegion( regionName2 ),
                region.toString() );

        // check cs.cl has been deleted
        String csName1 = RegionUtils
                .getDataCSName( regionName2, "year", new Date() );
        String csName2 = RegionUtils.getMetaCSName( regionName2 );
        Assert.assertFalse( RegionUtils.doesCSExist( csName1 ),
                "csName1 = " + csName1 );
        Assert.assertFalse( RegionUtils.doesCSExist( csName2 ),
                "csName2 = " + csName2 );
    }

    @AfterClass
    private void tearDown() throws Exception {
        if ( s3Client != null ) {
            s3Client.shutdown();
        }
        if ( runSuccess ) {
            RegionUtils.dropCS( new String[] { csName } );
        }
    }
}
