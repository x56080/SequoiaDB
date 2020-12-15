package com.sequoias3.region.concurrent;

import java.io.File;
import java.util.Date;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.amazonaws.services.s3.AmazonS3;
import com.sequoias3.SequoiaS3;
import com.sequoias3.common.DataShardingType;
import com.sequoias3.exception.SequoiaS3ServiceException;
import com.sequoias3.model.CreateRegionRequest;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.S3ThreadBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import com.sequoias3.testcommon.s3utils.RegionUtils;

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
    private DataShardingType oldShardingType = DataShardingType.YEAR;
    private DataShardingType newShardingType = DataShardingType.MONTH;
    private int fileSize = 1024 * 20;
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

        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        regionClient = CommLib.regionClient();
        RegionUtils.clearRegion( regionClient, regionName );

        CreateRegionRequest request = new CreateRegionRequest( regionName );
        request.withDataCSShardingType( oldShardingType )
                .withDataCLShardingType( oldShardingType );
        regionClient.createRegion( request );
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
                if ( regionClient.headRegion( regionName ) ) {
                    regionClient.deleteRegion( regionName );
                }
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            regionClient.shutdown();
            s3Client.shutdown();
        }
    }

    private void checkResult() throws Exception {
        boolean doesExistRegion = regionClient.headRegion( regionName );
        if ( doesExistRegion ) {
            RegionUtils.checkRegionWithShardingType( regionClient, regionName,
                    newShardingType, newShardingType );
            createObjectAndCheckResult();
        } else {
            // check that the auto create cs have been deleted
            String metaCSName = RegionUtils.getMetaCSName( regionName );
            String dataCSName = RegionUtils.getDataCSName( regionName,
                    DataShardingType.QUARTER, new Date() ) + "_1";
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
            SequoiaS3 regionClient1 = CommLib.regionClient();
            try {
                CreateRegionRequest request = new CreateRegionRequest(
                        regionName );
                request.withDataCSShardingType( newShardingType )
                        .withDataCLShardingType( newShardingType );
                regionClient1.createRegion( request );
            } finally {
                regionClient1.shutdown();
            }

        }
    }

    private class RemoveRegion extends S3ThreadBase {
        @Override
        public void exec() throws Exception {
            SequoiaS3 regionClient2 = CommLib.regionClient();
            try {
                regionClient2.deleteRegion( regionName );
            } catch ( SequoiaS3ServiceException e ) {
                if ( e.getStatusCode() != 404 ) {
                    throw e;
                }
            } finally {
                regionClient2.shutdown();
            }
        }
    }

}
