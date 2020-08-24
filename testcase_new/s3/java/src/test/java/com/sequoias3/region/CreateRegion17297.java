package com.sequoias3.region;

import java.io.File;
import java.util.concurrent.atomic.AtomicInteger;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import com.amazonaws.services.s3.AmazonS3;
import com.sequoias3.SequoiaS3;
import com.sequoias3.common.DataShardingType;
import com.sequoias3.model.CreateRegionRequest;
import com.sequoias3.model.GetRegionResult;
import com.sequoias3.model.Region;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import com.sequoias3.testcommon.s3utils.RegionUtils;

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
    private SequoiaS3 regionClient = null;

    @DataProvider(name = "regionProvider", parallel = true)
    public Object[][] generateRegion() {
        return new Object[][] {
                // the parameter : regionName and dataCLShardingType
                new Object[] { "region17297a", "year", DataShardingType.YEAR,
                        "bucket17297a" },
                new Object[] { "region17297b", "quarter",
                        DataShardingType.QUARTER, "bucket17297b" },
                new Object[] { "region17297c", "month", DataShardingType.MONTH,
                        "bucket17297c" } };
    }

    @BeforeClass
    private void setUp() throws Exception {
        s3Client = CommLib.buildS3Client();
        regionClient = CommLib.regionClient();
    }

    @Test(dataProvider = "regionProvider")
    public void testRegion( String regionName, String expdataCLShardingType,
            DataShardingType dataCLShardingType, String bucketName )
                    throws Exception {
        SequoiaS3 regionClient1 = CommLib.regionClient();
        RegionUtils.clearRegion( regionClient1, regionName );
        CreateRegionRequest request = new CreateRegionRequest( regionName );
        request.withDataCLShardingType( dataCLShardingType );
        regionClient1.createRegion( request );

        // get region and check region info
        checkRegion( regionName, expdataCLShardingType );

        // create object on region
        createObjectAndCheckResult( regionClient1, regionName, bucketName );
        actSuccessTests.getAndIncrement();
        regionClient1.shutdown();
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( actSuccessTests.get() == generateRegion().length ) {
                CommLib.clearBucket( s3Client, "bucket17297a" );
                CommLib.clearBucket( s3Client, "bucket17297b" );
                CommLib.clearBucket( s3Client, "bucket17297c" );
                regionClient.deleteRegion( "region17297a" );
                regionClient.deleteRegion( "region17297b" );
                regionClient.deleteRegion( "region17297c" );
            }
        } finally {
            regionClient.shutdown();
            s3Client.shutdown();
        }
    }

    private void checkRegion( String regionName, String dataCLShardingType )
            throws Exception {
        GetRegionResult result = regionClient.getRegion( regionName );
        Region regionInfo = result.getRegion();
        Assert.assertEquals( regionInfo.getDataCLShardingType().toString(),
                dataCLShardingType );
        // get the region infor to take the default value
        Assert.assertEquals( regionInfo.getDataCSShardingType().toString(),
                "year" );
        Assert.assertEquals( regionInfo.getMetaDomain(), null );
        Assert.assertEquals( regionInfo.getDataDomain(), null );
        Assert.assertEquals( regionInfo.getMetaLocation(), null );
        Assert.assertEquals( regionInfo.getMetaHisLocation(), null );
        Assert.assertEquals( regionInfo.getDataLocation(), null );
    }

    @SuppressWarnings("deprecation")
    private void createObjectAndCheckResult( SequoiaS3 regionClient,
            String regionName, String bucketName ) throws Exception {
        s3Client.createBucket( bucketName, regionName );
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );
        String context = "testcreatekeyonregion17297";
        String expMd5 = TestTools.getMD5( context.getBytes() );
        for ( int i = 0; i < 10; i++ ) {
            s3Client.putObject( bucketName, key, context );
            File localPath = new File( S3TestBase.workDir + File.separator
                    + TestTools.getClassName() + bucketName );
            String versionId = i + "";
            String downfileMd5 = ObjectUtils.getMd5OfObject( s3Client,
                    localPath, bucketName, key, versionId );
            Assert.assertEquals( downfileMd5, expMd5, "the fail version is "
                    + versionId + " bucket is " + bucketName );
            TestTools.LocalFile.removeFile( localPath );
        }
    }

}
