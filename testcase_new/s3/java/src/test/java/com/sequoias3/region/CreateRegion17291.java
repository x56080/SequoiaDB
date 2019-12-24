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
import org.testng.annotations.Test;

import java.io.File;

/**
 * @Description seqDB-17291: create Region and specify cs and cl.
 * @author wuyan
 * @Date 2019.1.18
 * @version 1.00
 */
public class CreateRegion17291 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket17291";
    private String key = "key17291";
    private String regionName = "region17291";
    private AmazonS3 s3Client = null;
    private String[] csNames = { "metaCS17291", "dataCS17291" };
    private String[] metaclNames = { "metaCL17291", "metaHistroyCL17291" };
    private String[] dataclNames = { "dataCL17291" };
    private int fileSize = 1024 * 1024 * 3;
    private File localPath = null;
    private String filePath = null;

    @BeforeClass
    private void setUp() throws Exception {
        localPath = new File( S3TestBase.workDir + File.separator + TestTools
                .getClassName() );
        filePath =
                localPath + File.separator + "localFile_" + fileSize + ".txt";

        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath, fileSize );

        RegionUtils.createCSAndCL( csNames[ 0 ], metaclNames );
        RegionUtils.createCSAndCL( csNames[ 1 ], dataclNames );

        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        RegionUtils.clearRegion( regionName );
    }

    @Test
    public void testRegion() throws Exception {
        Region region = new Region();
        String metaLocation = csNames[ 0 ] + "." + metaclNames[ 0 ];
        String metaHisLocation = csNames[ 0 ] + "." + metaclNames[ 1 ];
        String dataLocation = csNames[ 1 ] + "." + dataclNames[ 0 ];
        region.withMetaLocation( metaLocation ).withDataLocation( dataLocation )
                .withMetaHisLocation( metaHisLocation ).withName( regionName );
        RegionUtils.putRegion( region );
        // get region and check region info
        checkRegion( metaLocation, metaHisLocation, dataLocation );
        // create object on region
        createObjectAndCheckResult();
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( runSuccess ) {
                CommLib.clearBucket( s3Client, bucketName );
                RegionUtils.deleteRegion( regionName );
                RegionUtils.dropCS( csNames );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            s3Client.shutdown();
        }
    }

    private void checkRegion( String metaLocation, String metaHisLocation,
            String dataLocation ) throws Exception {
        GetRegionResult result = RegionUtils.getRegion( regionName );
        Region regionInfo = result.getRegion();
        Assert.assertEquals( regionInfo.getMetaLocation(), metaLocation );
        Assert.assertEquals( regionInfo.getMetaHisLocation(), metaHisLocation );
        Assert.assertEquals( regionInfo.getDataLocation(), dataLocation );
    }

    @SuppressWarnings("deprecation")
    private void createObjectAndCheckResult() throws Exception {
        s3Client.createBucket( bucketName, regionName );
        s3Client.putObject( bucketName, key, new File( filePath ) );
        String downfileMd5 = ObjectUtils
                .getMd5OfObject( s3Client, localPath, bucketName, key );
        Assert.assertEquals( downfileMd5, TestTools.getMD5( filePath ) );
    }

}
