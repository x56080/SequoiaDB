package com.sequoias3.region.concurrent;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.GetObjectRequest;
import com.amazonaws.services.s3.model.S3Object;
import com.amazonaws.services.s3.model.S3ObjectInputStream;
import com.sequoias3.SequoiaS3;
import com.sequoias3.common.DataShardingType;
import com.sequoias3.model.CreateRegionRequest;
import com.sequoias3.model.GetRegionResult;
import com.sequoias3.model.Region;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.S3ThreadBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import com.sequoias3.testcommon.s3utils.RegionUtils;

/**
 * @Description seqDB-17335:并发更新区域和使用区域
 * @author wangkexin
 * @Date 2019.01.29
 * @version 1.00
 */

public class UpdateAndUseRegion17335 extends S3TestBase {
    private String regionName = "Beijing17335";
    private String bucketName = "bucket17335";
    private String keyName = "object17335";
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * 1024;
    private File localPath = null;
    private String filePath = null;
    private int objectNums = 20;
    private boolean runSuccess = false;
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

        regionClient.createRegion( regionName );
        s3Client.createBucket( new CreateBucketRequest( bucketName,
                regionName.toLowerCase() ) );
    }

    @Test
    public void testCreateRegion() throws Exception {
        UpdateRegionThread updateRegion = new UpdateRegionThread();
        List< CreateAndGetObjectThread > createAndGetObjs = new ArrayList< >(
                objectNums );

        for ( int i = 0; i < objectNums; i++ ) {
            String key = keyName + "_" + i;
            createAndGetObjs.add( new CreateAndGetObjectThread( key ) );
        }
        for ( CreateAndGetObjectThread createAndGetObjThread : createAndGetObjs ) {
            createAndGetObjThread.start();
        }
        updateRegion.start();

        for ( CreateAndGetObjectThread createAndGetObjThread : createAndGetObjs ) {
            Assert.assertTrue( createAndGetObjThread.isSuccess(),
                    createAndGetObjThread.getErrorMsg() );
        }

        Assert.assertTrue( updateRegion.isSuccess(),
                updateRegion.getErrorMsg() );
        checkUpdate();
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( runSuccess ) {
                CommLib.clearBucket( s3Client, bucketName );
                regionClient.deleteRegion( regionName );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            if ( regionClient != null ) {
                regionClient.shutdown();
            }
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }

    private void checkResult( AmazonS3 s3Client, String keyName )
            throws Exception {
        GetObjectRequest request = new GetObjectRequest( bucketName, keyName );
        S3Object object = s3Client.getObject( request );
        Assert.assertEquals( object.getKey(), keyName );

        S3ObjectInputStream s3is = object.getObjectContent();
        String downloadPath = TestTools.LocalFile.initDownloadPath( localPath,
                TestTools.getMethodName(), Thread.currentThread().getId() );
        ObjectUtils.inputStream2File( s3is, downloadPath );
        s3is.close();
        String downfileMd5 = TestTools.getMD5( downloadPath );
        Assert.assertEquals( downfileMd5, TestTools.getMD5( filePath ) );
    }

    private void checkUpdate() throws Exception {
        GetRegionResult result = regionClient.getRegion( regionName );
        String actBucketName = result.getBuckets().get( 0 );
        Assert.assertEquals( actBucketName, bucketName );
        Region region = result.getRegion();
        Assert.assertEquals( region.getDataCSShardingType(),
                DataShardingType.QUARTER );
        Assert.assertEquals( region.getDataCLShardingType(),
                DataShardingType.MONTH );
    }

    private class UpdateRegionThread extends S3ThreadBase {
        @Override
        public void exec() throws Exception {
            SequoiaS3 regionClientNew = CommLib.regionClient();
            CreateRegionRequest request = new CreateRegionRequest( regionName );
            request.withDataCSShardingType( DataShardingType.QUARTER )
                    .withDataCLShardingType( DataShardingType.MONTH );
            regionClientNew.createRegion( request );
            regionClientNew.shutdown();
        }
    }

    private class CreateAndGetObjectThread extends S3ThreadBase {
        private String keyName;

        public CreateAndGetObjectThread( String keyName ) {
            this.keyName = keyName;
        }

        @Override
        public void exec() throws Exception {
            AmazonS3 s3Client = CommLib.buildS3Client();
            try {
                s3Client.putObject( bucketName, keyName, new File( filePath ) );
                checkResult( s3Client, keyName );
            } finally {
                if ( s3Client != null ) {
                    s3Client.shutdown();
                }
            }
        }
    }
}
