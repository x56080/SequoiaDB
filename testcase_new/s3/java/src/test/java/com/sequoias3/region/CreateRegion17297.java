package com.sequoias3.region;

import com.amazonaws.services.s3.AmazonS3;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import com.sequoias3.testcommon.s3utils.RegionUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import java.io.File;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * @Description seqDB-17297: create Region and specify set DataCLShardingType.
 * @author wuyan
 * @Date 2019.1.22
 * @version 1.00
 */
public class CreateRegion17297 extends S3TestBase {
    private AtomicInteger actSuccessTests = new AtomicInteger( 0 );
    private String key = "key17297";
    private AmazonS3 s3Client = null;

    @DataProvider(name = "regionProvider", parallel = true)
    public Object[][] generateRegion() {
        return new Object[][] {
                // the parameter : regionName and dataCLShardingType
                new Object[] { "region17297a", "year", "bucket17297a" },
                new Object[] { "region17297b", "quarter", "bucket17297b" },
                new Object[] { "region17297c", "month", "bucket17297c" } };
    }

    @BeforeClass
    private void setUp() throws Exception {
        s3Client = CommLib.buildS3Client();
    }

    @Test(dataProvider = "regionProvider")
    public void testRegion( String regionName, String dataCLShardingType,
            String bucketName ) throws Exception {
        RegionUtils.clearRegion( regionName );

        Region region = new Region();
        region.withDataCLShardingType( dataCLShardingType )
                .withName( regionName );
        RegionUtils.putRegion( region );

        // get region and check region info
        checkRegion( regionName, dataCLShardingType );

        // create object on region
        createObjectAndCheckResult( regionName, bucketName );
        actSuccessTests.getAndIncrement();
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( actSuccessTests.get() == generateRegion().length ) {
                CommLib.clearBucket( s3Client, "bucket17297a" );
                CommLib.clearBucket( s3Client, "bucket17297b" );
                CommLib.clearBucket( s3Client, "bucket17297c" );
                RegionUtils.deleteRegion( "region17297a" );
                RegionUtils.deleteRegion( "region17297b" );
                RegionUtils.deleteRegion( "region17297c" );
            }
        } finally {
            s3Client.shutdown();
        }
    }

    private void checkRegion( String regionName, String dataCLShardingType )
            throws Exception {
        GetRegionResult result = RegionUtils.getRegion( regionName );
        Region regionInfo = result.getRegion();
        Assert.assertEquals( regionInfo.getDataCLShardingType(),
                dataCLShardingType );
        // get the region infor to take the default value
        Assert.assertEquals( regionInfo.getDataCSShardingType(), "year" );
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
        String context = "testcreatekeyonregion17297";
        String expMd5 = TestTools.getMD5( context.getBytes() );
        for ( int i = 0; i < 10; i++ ) {
            s3Client.putObject( bucketName, key, context );
            File localPath = new File(
                    S3TestBase.workDir + File.separator + TestTools
                            .getClassName() + bucketName );
            // File localPath = new File(S3TestBase.workDir + File.separator +
            // TestTools.getClassName() );
            String versionId = i + "";
            String downfileMd5 = ObjectUtils
                    .getMd5OfObject( s3Client, localPath, bucketName, key,
                            versionId );
            Assert.assertEquals( downfileMd5, expMd5,
                    "the fail version is " + versionId + " bucket is "
                            + bucketName );
            TestTools.LocalFile.removeFile( localPath );
        }
        s3Client.shutdown();
    }

}
