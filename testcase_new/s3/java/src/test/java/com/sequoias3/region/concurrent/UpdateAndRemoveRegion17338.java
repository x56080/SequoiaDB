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
import java.util.Date;

/**
 * @Description seqDB-17338: concurrent update region and remove region.
 * @author wuyan
 * @Date 2019.1.30
 * @version 1.00
 */
public class UpdateAndRemoveRegion17338 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket17338";
    private String key = "key17338";
    private String regionName = "region17338";
    private AmazonS3 s3Client = null;
    private String oldShardingType = "year";
    private String newShardingType = "month";
    private int fileSize = 1024 * 20;
    private File localPath = null;
    private String filePath = null;

    @BeforeClass
    private void setUp() throws Exception {
        localPath = new File( S3TestBase.workDir + File.separator
                + TestTools.getClassName() );
        filePath = localPath + File.separator + "localFile_" + fileSize
                + ".txt";

        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath, fileSize );

        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        RegionUtils.clearRegion( regionName );
        Region region = new Region();
        region.withDataCSShardingType( oldShardingType )
                .withDataCLShardingType( oldShardingType )
                .withName( regionName );
        RegionUtils.putRegion( region );
    }

    @Test
    public void testRegion() throws Exception {
        UpdateRegion updateRegion = new UpdateRegion();
        RemoveRegion removeRegion = new RemoveRegion();
        updateRegion.start( 10 );
        removeRegion.start( 10 );
        Assert.assertTrue( updateRegion.isSuccess(),
                updateRegion.getErrorMsg() );
        Assert.assertTrue( removeRegion.isSuccess(),
                removeRegion.getErrorMsg() );
        checkResult();
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( runSuccess ) {
                CommLib.clearBucket( s3Client, bucketName );
                if ( RegionUtils.headRegion( regionName ) ) {
                    RegionUtils.deleteRegion( regionName );
                }
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            s3Client.shutdown();
        }
    }

    private void checkResult() throws Exception {
        boolean doesExistRegion = RegionUtils.headRegion( regionName );
        if ( doesExistRegion ) {
            RegionUtils.checkRegionWithShardingType( regionName,
                    newShardingType, newShardingType );
            createObjectAndCheckResult();
        } else {
            // check that the auto create cs have been deleted
            String metaCSName = RegionUtils.getMetaCSName( regionName );
            String dataCSName = RegionUtils.getDataCSName( regionName,
                    "quarter", new Date() ) + "_1";
            Assert.assertFalse( RegionUtils.doesCSExist( metaCSName ) );
            Assert.assertFalse( RegionUtils.doesCSExist( dataCSName ) );
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

    private class UpdateRegion extends S3ThreadBase {
        @Override
        public void exec() throws Exception {
            Region region = new Region();
            region.withDataCSShardingType( newShardingType )
                    .withDataCLShardingType( newShardingType )
                    .withName( regionName );
            RegionUtils.putRegion( region );
        }
    }

    private class RemoveRegion extends S3ThreadBase {
        @Override
        public void exec() throws Exception {
            try {
                RegionUtils.deleteRegion( regionName );
            } catch ( AmazonS3Exception e ) {
                if ( e.getStatusCode() != 404 ) {
                    throw e;
                }
            }
        }
    }

}
