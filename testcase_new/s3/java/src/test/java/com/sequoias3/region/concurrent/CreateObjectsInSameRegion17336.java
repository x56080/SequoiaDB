package com.sequoias3.region.concurrent;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.GetObjectRequest;
import com.amazonaws.services.s3.model.S3Object;
import com.amazonaws.services.s3.model.S3ObjectInputStream;
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
import java.util.ArrayList;
import java.util.List;

/**
 * test content: 相同区域并发创建对象 testlink-case: seqDB-17336
 *
 * @author wangkexin
 * @Date 2019.01.29
 * @version 1.00
 */

public class CreateObjectsInSameRegion17336 extends S3TestBase {
    private String regionName = "Beijing17336";
    private String bucketName = "bucket17336";
    private String keyName = "object17336";
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * 1024;
    private File localPath = null;
    private String filePath = null;
    private int objectNums = 50;
    private boolean runSuccess = false;

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
        region.withName( regionName );
        RegionUtils.putRegion( region );
        s3Client.createBucket( new CreateBucketRequest( bucketName,
                regionName.toLowerCase() ) );
    }

    @Test
    public void testCreateRegion() throws Exception {
        List< CreateObjectThread > createObjs = new ArrayList<>( objectNums );

        for ( int i = 0; i < objectNums; i++ ) {
            String key = keyName + "_" + i;
            createObjs.add( new CreateObjectThread( key ) );
        }
        for ( CreateObjectThread createObjThread : createObjs ) {
            createObjThread.start();
        }
        for ( CreateObjectThread createObjThread : createObjs ) {
            Assert.assertTrue( createObjThread.isSuccess(),
                    createObjThread.getErrorMsg() );
        }

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( runSuccess ) {
                CommLib.clearBucket( s3Client, bucketName );
                RegionUtils.deleteRegion( regionName );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
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

    private class CreateObjectThread extends S3ThreadBase {
        private String keyName;

        public CreateObjectThread( String keyName ) {
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
