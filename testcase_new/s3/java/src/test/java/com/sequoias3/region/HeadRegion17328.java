package com.sequoias3.region;

import java.util.Date;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.sequoiadb.base.Sequoiadb;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.bean.Region;
import com.sequoias3.testcommon.s3utils.RegionUtils;

/**
 * test content: Head查询区域信息 testlink-case: seqDB-17328
 *
 * @author wangkexin
 * @Date 2019.01.25
 * @version 1.00
 */

public class HeadRegion17328 extends S3TestBase {
    private static Sequoiadb sdb = null;
    String dataCSName = null;
    String metaCSName = null;
    private String regionName = "Beijing17328";
    private String bucketName = "bucket17328";
    private AmazonS3 s3Client = null;
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {
        s3Client = CommLib.buildS3Client();
        sdb = new Sequoiadb( S3TestBase.coordUrl, "", "" );

        dataCSName = RegionUtils.getDataCSName( regionName.toLowerCase(),
                "year", new Date() ) + "_1";
        metaCSName = RegionUtils.getMetaCSName( regionName.toLowerCase() );

        if ( sdb.isCollectionSpaceExist( dataCSName ) ) {
            sdb.dropCollectionSpace( dataCSName );
        }
        if ( sdb.isCollectionSpaceExist( metaCSName ) ) {
            sdb.dropCollectionSpace( metaCSName );
        }

        CommLib.clearBucket( s3Client, bucketName );
        RegionUtils.clearRegion( regionName );

        // create region
        Region region = new Region();
        region.withName( regionName );
        RegionUtils.putRegion( region );

        s3Client.createBucket( new CreateBucketRequest( bucketName,
                regionName.toLowerCase() ) );
        s3Client.putObject( bucketName, "key17328", "content17328" );
    }

    @Test
    public void testCreateRegion() throws Exception {
        Assert.assertTrue( RegionUtils.headRegion( regionName ) );
        CommLib.clearBucket( s3Client, bucketName );
        sdb.dropCollectionSpace( dataCSName );
        sdb.dropCollectionSpace( metaCSName );

        RegionUtils.deleteRegion( regionName );
        Assert.assertFalse( RegionUtils.headRegion( regionName ) );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        if ( runSuccess ) {
            sdb.close();
        }
    }
}
