package com.sequoias3.object.concurrent;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.BucketVersioningConfiguration;
import com.amazonaws.services.s3.model.S3Object;
import com.amazonaws.services.s3.model.S3ObjectInputStream;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.S3ThreadBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;

/**
 * @Description seqDB-16514:Suspended bucket versioning,concurrent get and
 *              update the same object
 * @author wuyan
 * @Date 2019.1.8
 * @version 1.00
 */
public class UpdateAndGetSameObject16514 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket16514";
    private String keyName = "/dir-1/bb/object16514";
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * 1024 * 2;
    private int updateSize = 1024 * 3;
    private File localPath = null;
    private String filePath = null;
    private String updatePath = null;
    private long objectLength = 0;
    private String getObjectMd5 = "";

    @BeforeClass
    private void setUp() throws Exception {
        localPath = new File( S3TestBase.workDir + File.separator
                + TestTools.getClassName() );
        filePath = localPath + File.separator + "localFile_" + fileSize
                + ".txt";
        updatePath = localPath + File.separator + "localFile_" + updateSize
                + ".txt";
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath, fileSize );
        TestTools.LocalFile.createFile( updatePath, updateSize );

        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
        CommLib.setBucketVersioning( s3Client, bucketName,
                BucketVersioningConfiguration.SUSPENDED );
        s3Client.putObject( bucketName, keyName, new File( filePath ) );
    }

    @Test
    public void testObject() throws Exception {
        GetObjectThread getObjectThread = new GetObjectThread();
        UpdateObjectThread updateObjectThread = new UpdateObjectThread();
        updateObjectThread.start();
        getObjectThread.start();

        if ( updateObjectThread.isSuccess() ) {

            if ( getObjectThread.isSuccess() ) {
                checkGetObject( bucketName, keyName );
            } else {
                AmazonS3Exception e = ( AmazonS3Exception ) ( getObjectThread
                        .getExceptions().get( 0 ) );
                if ( !e.getErrorCode().equals( "NoSuchKey" ) ) {
                    Assert.fail(
                            "getObject fail:" + getObjectThread.getErrorMsg()
                                    + "  e:" + e.getErrorCode() );
                }
            }
            checkUpdateObjectResult( bucketName, keyName );
        } else {
            Assert.fail( "Unexpected results! updateObjectError:"
                    + updateObjectThread.getErrorMsg() + "getObjectError:"
                    + getObjectThread.getErrorMsg() );
        }

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( runSuccess ) {
                CommLib.clearBucket( s3Client, bucketName );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }

    private void checkGetObject( String bucketName, String key )
            throws Exception {
        // check get object result from md5
        if ( objectLength == fileSize ) {
            Assert.assertEquals( getObjectMd5, TestTools.getMD5( filePath ) );
        } else {
            Assert.assertEquals( getObjectMd5, TestTools.getMD5( updatePath ) );
        }
    }

    private void checkUpdateObjectResult( String bucketName, String key )
            throws Exception {
        String downfileMd5 = ObjectUtils.getMd5OfObject( s3Client, localPath,
                bucketName, key );
        Assert.assertEquals( downfileMd5, TestTools.getMD5( updatePath ) );
    }

    private class UpdateObjectThread extends S3ThreadBase {
        @Override
        public void exec() throws Exception {
            AmazonS3 s3Client = CommLib.buildS3Client();
            try {
                s3Client.putObject( bucketName, keyName,
                        new File( updatePath ) );
            } finally {
                if ( s3Client != null ) {
                    s3Client.shutdown();
                }
            }
        }
    }

    private class GetObjectThread extends S3ThreadBase {
        @Override
        public void exec() throws Exception {
            AmazonS3 s3Client = CommLib.buildS3Client();
            try {
                S3Object object = s3Client.getObject( bucketName, keyName );
                objectLength = object.getObjectMetadata().getContentLength();
                S3ObjectInputStream s3is = object.getObjectContent();
                String downloadPath = TestTools.LocalFile.initDownloadPath(
                        localPath, TestTools.getMethodName(),
                        Thread.currentThread().getId() );
                ObjectUtils.inputStream2File( s3is, downloadPath );
                s3is.close();
                getObjectMd5 = TestTools.getMD5( downloadPath );
            } finally {
                if ( s3Client != null ) {
                    s3Client.shutdown();
                }
            }
        }
    }

}
