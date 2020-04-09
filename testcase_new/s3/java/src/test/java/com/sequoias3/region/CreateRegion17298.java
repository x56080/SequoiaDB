package com.sequoias3.region;

import java.io.File;
import java.util.concurrent.atomic.AtomicInteger;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import com.amazonaws.services.s3.AmazonS3;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import com.sequoias3.testcommon.s3utils.RegionUtils;
import com.sequoias3.testcommon.s3utils.bean.GetRegionResult;
import com.sequoias3.testcommon.s3utils.bean.Region;

/**
 * @Description seqDB-17298: create Region and specify set DataCSShardingType.
 * @author wuyan
 * @Date 2019.1.22
 * @version 1.00
 */
public class CreateRegion17298 extends S3TestBase {
    private AtomicInteger actSuccessTests = new AtomicInteger( 0 );
    private String key = "key17298";
    private AmazonS3 s3Client = null;

    @DataProvider(name = "regionProvider")
    public Object[][] generateRegion() {
        return new Object[][] {
                // the parameter : regionName and dataCSShardingType
                new Object[] { "region17298a", "year", "bucket17298a" },
                new Object[] { "region17298b", "quarter", "bucket17298b" },
                new Object[] { "region17298c", "month", "bucket17298c" } };
    }

    @BeforeClass
    private void setUp() throws Exception {
        s3Client = CommLib.buildS3Client();
    }

    @Test(dataProvider = "regionProvider")
    public void testRegion( String regionName, String dataCSShardingType,
            String bucketName ) throws Exception {
        RegionUtils.clearRegion( regionName );

        Region region = new Region();
        region.withDataCSShardingType( dataCSShardingType )
                .withName( regionName );
        RegionUtils.putRegion( region );

        // get region and check region info
        checkRegion( regionName, dataCSShardingType );

        // create object on region
        createObjectAndCheckResult( regionName, bucketName );
        actSuccessTests.getAndIncrement();
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( actSuccessTests.get() == generateRegion().length ) {
                CommLib.clearBucket( s3Client, "bucket17298a" );
                CommLib.clearBucket( s3Client, "bucket17298b" );
                CommLib.clearBucket( s3Client, "bucket17298c" );
                RegionUtils.deleteRegion( "region17298a" );
                RegionUtils.deleteRegion( "region17298b" );
                RegionUtils.deleteRegion( "region17298c" );
            }
        } finally {
            s3Client.shutdown();
        }
    }

    private void checkRegion( String regionName, String dataCSShardingType )
            throws Exception {
        GetRegionResult result = RegionUtils.getRegion( regionName );
        Region regionInfo = result.getRegion();
        Assert.assertEquals( regionInfo.getDataCSShardingType(),
                dataCSShardingType );
        // get the region infor to take the default value
        Assert.assertEquals( regionInfo.getDataCLShardingType(), "quarter" );
        Assert.assertEquals( regionInfo.getMetaDomain(), "" );
        Assert.assertEquals( regionInfo.getDataDomain(), "" );
        Assert.assertEquals( regionInfo.getMetaLocation(), "" );
        Assert.assertEquals( regionInfo.getMetaHisLocation(), "" );
        Assert.assertEquals( regionInfo.getDataLocation(), "" );
    }

    @SuppressWarnings("deprecation")
    private void createObjectAndCheckResult( String regionName,
            String bucketName ) throws Exception {
        AmazonS3 s3Client = CommLib.buildS3Client();
        s3Client.createBucket( bucketName, regionName );
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );
        for ( int i = 0; i < 10; i++ ) {
            String context = "testcreatekeyonregion17298_" + bucketName + "_"
                    + i;
            s3Client.putObject( bucketName, key, context );
            File localPath = new File( S3TestBase.workDir + File.separator
                    + TestTools.getClassName() + bucketName );
            String versionId = i + "";
            String downfileMd5 = ObjectUtils.getMd5OfObject( s3Client,
                    localPath, bucketName, key, versionId );
            Assert.assertEquals( downfileMd5,
                    TestTools.getMD5( context.getBytes() ),
                    "the bucket is " + bucketName + ", the versionId is " + i
                            + ",the context:" + context );
            TestTools.LocalFile.removeFile( localPath );
        }
        s3Client.shutdown();
    }

}
