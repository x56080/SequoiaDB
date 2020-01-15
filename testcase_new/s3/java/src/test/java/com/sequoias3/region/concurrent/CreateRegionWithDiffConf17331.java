package com.sequoias3.region.concurrent;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.sequoias3.testcommon.s3utils.bean.Region;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.S3ThreadBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import com.sequoias3.testcommon.s3utils.RegionUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;

/**
 * @Description seqDB-17331: concurrent create Region and specify different
 *              configuration mode.
 * @author wuyan
 * @Date 2019.1.30
 * @version 1.00
 */
public class CreateRegionWithDiffConf17331 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket17331";
    private String key = "key17331";
    private String regionName = "region17331";
    private AmazonS3 s3Client = null;
    private String[] csNames = { "metaCS17331", "dataCS17331" };
    private String[] metaclNames = { "metaCL17331", "metaHistroyCL17331" };
    private String[] dataclNames = { "dataCL17331" };
    private String metaLocation = csNames[ 0 ] + "." + metaclNames[ 0 ];
    private String metaHisLocation = csNames[ 0 ] + "." + metaclNames[ 1 ];
    private String dataLocation = csNames[ 1 ] + "." + dataclNames[ 0 ];
    private String shardingType = "month";
    private int fileSize = 1024 * 10;
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
    public void checkResult() throws Exception {
        PutRegionWithSpecifyCSCL putRegionWithSpecifyCSCL = new PutRegionWithSpecifyCSCL();
        PutRegionWithDynamic putRegionWithDynamic = new PutRegionWithDynamic();
        putRegionWithDynamic.start();
        putRegionWithSpecifyCSCL.start();
        if ( putRegionWithDynamic.isSuccess() && !putRegionWithSpecifyCSCL
                .isSuccess() ) {
            AmazonS3Exception e = ( AmazonS3Exception ) ( putRegionWithSpecifyCSCL
                    .getExceptions().get( 0 ) );
            // 409:ConflictRegionType
            if ( e.getStatusCode() != 409 ) {
                Assert.fail( "put region with specifycscl fail:" + e
                        .getErrorMessage() + "/n e:" + e.getStatusCode() );
            }
            RegionUtils.checkRegionWithShardingType( regionName, shardingType,
                    shardingType );
        } else if ( putRegionWithSpecifyCSCL.isSuccess()
                && !putRegionWithDynamic.isSuccess() ) {
            AmazonS3Exception e = ( AmazonS3Exception ) ( putRegionWithDynamic
                    .getExceptions().get( 0 ) );
            // 409:ConflictRegionType
            if ( e.getStatusCode() != 409 ) {
                Assert.fail(
                        "put region with dynamic fail:" + e.getErrorMessage()
                                + "/n e:" + e.getStatusCode() );
            }
            RegionUtils.checkRegionWithLocation( regionName, metaLocation,
                    metaHisLocation, dataLocation );
        } else {
            Assert.fail( "unexpected results!" );
        }

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

    @SuppressWarnings("deprecation")
    private void createObjectAndCheckResult() throws Exception {
        s3Client.createBucket( bucketName, regionName );
        s3Client.putObject( bucketName, key, new File( filePath ) );
        String downfileMd5 = ObjectUtils
                .getMd5OfObject( s3Client, localPath, bucketName, key );
        Assert.assertEquals( downfileMd5, TestTools.getMD5( filePath ) );
    }

    private class PutRegionWithSpecifyCSCL extends S3ThreadBase {
        @Override
        public void exec() throws Exception {
            Region region = new Region();
            region.withMetaLocation( metaLocation )
                    .withDataLocation( dataLocation )
                    .withMetaHisLocation( metaHisLocation )
                    .withName( regionName );
            RegionUtils.putRegion( region );
        }
    }

    private class PutRegionWithDynamic extends S3ThreadBase {
        @Override
        public void exec() throws Exception {
            Region region = new Region();
            region.withDataCLShardingType( shardingType )
                    .withDataCSShardingType( shardingType )
                    .withName( regionName );
            RegionUtils.putRegion( region );
        }
    }

}
