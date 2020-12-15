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
 * @Description seqDB-17298: create Region and specify set DataCSShardingType.
 * @author wuyan
 * @Date 2019.1.22
 * @version 1.00
 */
public class CreateRegion17298 extends S3TestBase {
    private AtomicInteger actSuccessTests = new AtomicInteger( 0 );
    private String key = "key17298";
    private AmazonS3 s3Client = null;
    private SequoiaS3 regionClient = null;

    @DataProvider(name = "regionProvider")
    public Object[][] generateRegion() {
        return new Object[][] {
                // the parameter : regionName and dataCSShardingType
                new Object[] { "region17298a", DataShardingType.YEAR,
                        "bucket17298a" },
                new Object[] { "region17298b", DataShardingType.QUARTER,
                        "bucket17298b" },
                new Object[] { "region17298c", DataShardingType.MONTH,
                        "bucket17298c" } };
    }

    @BeforeClass
    private void setUp() throws Exception {
        s3Client = CommLib.buildS3Client();
        regionClient = CommLib.regionClient();
    }

    @Test(dataProvider = "regionProvider")
    public void testRegion( String regionName,
            DataShardingType dataCSShardingType, String bucketName )
                    throws Exception {
        SequoiaS3 regionClientNew = CommLib.regionClient();
        RegionUtils.clearRegion( regionClient, regionName );

        CreateRegionRequest request = new CreateRegionRequest( regionName );
        request.withDataCSShardingType( dataCSShardingType );
        regionClient.createRegion( request );

        // get region and check region info
        checkRegion( regionClientNew, regionName, dataCSShardingType );

        // create object on region
        createObjectAndCheckResult( regionName, bucketName );
        actSuccessTests.getAndIncrement();
        regionClientNew.shutdown();

    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( actSuccessTests.get() == generateRegion().length ) {
                CommLib.clearBucket( s3Client, "bucket17298a" );
                CommLib.clearBucket( s3Client, "bucket17298b" );
                CommLib.clearBucket( s3Client, "bucket17298c" );
                regionClient.deleteRegion( "region17298a" );
                regionClient.deleteRegion( "region17298b" );
                regionClient.deleteRegion( "region17298c" );
            }
        } finally {
            regionClient.shutdown();
            s3Client.shutdown();
        }
    }

    private void checkRegion( SequoiaS3 regionClient, String regionName,
            DataShardingType dataCSShardingType ) throws Exception {
        GetRegionResult result = regionClient.getRegion( regionName );
        Region regionInfo = result.getRegion();
        Assert.assertEquals( regionInfo.getDataCSShardingType(),
                dataCSShardingType );
        // get the region infor to take the default value
        Assert.assertEquals( regionInfo.getDataCLShardingType(),
                DataShardingType.QUARTER );
        Assert.assertEquals( regionInfo.getMetaDomain(), null );
        Assert.assertEquals( regionInfo.getDataDomain(), null );
        Assert.assertEquals( regionInfo.getMetaLocation(), null );
        Assert.assertEquals( regionInfo.getMetaHisLocation(), null );
        Assert.assertEquals( regionInfo.getDataLocation(), null );
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
