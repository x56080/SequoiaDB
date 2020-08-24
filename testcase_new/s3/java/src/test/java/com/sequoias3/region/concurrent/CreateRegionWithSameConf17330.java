package com.sequoias3.region.concurrent;

import java.io.File;
import java.util.Collections;
import java.util.List;
import java.util.concurrent.atomic.AtomicInteger;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.amazonaws.services.s3.AmazonS3;
import com.sequoias3.SequoiaS3;
import com.sequoias3.model.CreateRegionRequest;
import com.sequoias3.model.ListRegionsResult;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import com.sequoias3.testcommon.s3utils.RegionUtils;

/**
 * @Description seqDB-17330: concurrent create Region and specify the same cs
 *              and cl.
 * @author wuyan
 * @Date 2019.1.29
 * @version 1.00
 */
public class CreateRegionWithSameConf17330 extends S3TestBase {
    private AtomicInteger actSuccessTests = new AtomicInteger( 0 );
    private String bucketName = "bucket17330";
    private String key = "key17330";
    private String regionName = "region17330";
    private AmazonS3 s3Client = null;
    private String[] csNames = { "metaCS17330", "dataCS17330" };
    private String[] metaclNames = { "metaCL17330", "metaHistroyCL17330" };
    private String[] dataclNames = { "dataCL17330" };
    private int fileSize = 1024 * 3;
    private File localPath = null;
    private String filePath = null;
    private SequoiaS3 regionClient = null;

    @BeforeClass
    private void setUp() throws Exception {
        localPath = new File( S3TestBase.workDir + File.separator
                + TestTools.getClassName() );
        filePath = localPath + File.separator + "localFile_" + fileSize
                + ".txt";

        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath, fileSize );

        RegionUtils.createCSAndCL( csNames[ 0 ], metaclNames );
        RegionUtils.createCSAndCL( csNames[ 1 ], dataclNames );

        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        regionClient = CommLib.regionClient();
        RegionUtils.clearRegion( regionClient, regionName );
    }

    @Test(threadPoolSize = 100, invocationCount = 100)
    public void testRegion() throws Exception {
        String metaLocation = csNames[ 0 ] + "." + metaclNames[ 0 ];
        String metaHisLocation = csNames[ 0 ] + "." + metaclNames[ 1 ];
        String dataLocation = csNames[ 1 ] + "." + dataclNames[ 0 ];
        CreateRegionRequest request = new CreateRegionRequest( regionName );
        request.withMetaLocation( metaLocation )
                .withDataLocation( dataLocation )
                .withMetaHisLocation( metaHisLocation );
        regionClient.createRegion( request );

        // get region and check region info
        RegionUtils.checkRegionWithLocation( regionClient, regionName,
                metaLocation, metaHisLocation, dataLocation );
        actSuccessTests.getAndIncrement();
    }

    @Test(dependsOnMethods = "testRegion")
    public void checkResult() throws Exception {
        ListRegionsResult result = regionClient.listRegions();
        List< String > listRegions = result.getRegions();
        int count = Collections.frequency( listRegions, regionName );
        // finally only create 1 region
        Assert.assertEquals( count, 1 );

        // create object on region
        createObjectAndCheckResult();
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( actSuccessTests.get() == 101 ) {
                CommLib.clearBucket( s3Client, bucketName );
                regionClient.deleteRegion( regionName );
                RegionUtils.dropCS( csNames );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            regionClient.shutdown();
            s3Client.shutdown();
        }
    }

    @SuppressWarnings("deprecation")
    private void createObjectAndCheckResult() throws Exception {
        s3Client.createBucket( bucketName, regionName );
        s3Client.putObject( bucketName, key, new File( filePath ) );
        String downfileMd5 = ObjectUtils.getMd5OfObject( s3Client, localPath,
                bucketName, key );
        Assert.assertEquals( downfileMd5, TestTools.getMD5( filePath ) );
    }

}
