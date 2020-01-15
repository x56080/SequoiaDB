package com.sequoias3.region;

import java.util.Calendar;
import java.util.Date;
import java.util.UUID;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.bean.Region;
import com.sequoias3.testcommon.s3utils.RegionUtils;

/**
 * @Description: seqDB-17325 :: 区域中存在桶，删除区域
 * @author fanyu
 * @Date:2019年01月22日
 * @version:1.0
 */
public class DeleteRegion17325 extends S3TestBase {
    private String regionName = "region17325";
    private String bucketName = "bucket17325";
    private String objectName = "object17325";
    private AmazonS3 s3Client = null;
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        RegionUtils.clearRegion( regionName );
    }

    @Test
    private void test() throws Exception {
        // create region
        Region region = new Region();
        region.withDataCSShardingType( "month" )
                .withDataCLShardingType( "month" ).withName( regionName );
        RegionUtils.putRegion( region );

        // create bucket and object
        s3Client.createBucket(
                new CreateBucketRequest( bucketName, regionName ) );
        s3Client.putObject( bucketName, objectName,
                String.valueOf( UUID.randomUUID() ) );

        // delete region
        try {
            RegionUtils.deleteRegion( regionName );
            Assert.fail(
                    "exp delete region failed,regionName = " + regionName );
        } catch ( AmazonS3Exception e ) {
            if ( e.getStatusCode() != 409 && !e.getErrorCode()
                    .contains( "RegionNotEmpty" ) ) {
                throw e;
            }
        }

        // head region to make sure the region:regionName has not been deleted
        Assert.assertTrue( RegionUtils.headRegion( regionName ),
                region.toString() );

        // check cs.cl has not been deleted
        Date date = Calendar.getInstance().getTime();
        String csName =
                RegionUtils.getDataCSName( regionName, "month", date ) + "_1";
        String clName = RegionUtils.getDataCLName( "month", date );
        Assert.assertTrue( RegionUtils.clInCS( csName, clName ),
                "csName = " + csName + ",clName = " + clName );
        // DataCL must have a lob
        Assert.assertEquals( RegionUtils.getRecordNum( csName, clName ), 1,
                "csNmae = " + csName + "clName = " + clName + ",objectName = "
                        + objectName );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( runSuccess ) {
                CommLib.clearBucket( s3Client, bucketName );
                RegionUtils.deleteRegion( regionName );
            }
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }
}
