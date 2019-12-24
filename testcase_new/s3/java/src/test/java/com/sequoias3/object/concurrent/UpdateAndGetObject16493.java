package com.sequoias3.object.concurrent;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.S3Object;
import com.amazonaws.services.s3.model.S3ObjectInputStream;
import com.sequoiadb.exception.BaseException;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.S3ThreadBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import com.sequoias3.testcommon.s3utils.UserUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;

/**
 * test content: 并发更新和获取对象 testlink-case: seqDB-16493
 *
 * @author wangkexin
 * @Date 2019.01.04
 * @version 1.00
 */
public class UpdateAndGetObject16493 extends S3TestBase {
    private boolean runSuccess = false;
    private String userName = "user16493";
    private String bucketName = "bucket16493";
    private String keyName = "key16493";
    private String roleName = "normal";
    private int fileSize = 1024 * 1024 * 2;
    private int updateSize = 1024 * 3;
    private File localPath = null;
    private String filePath = null;
    private String updatePath = null;
    private long objectLength = 0;
    private String getObjectMd5 = "";
    private String[] acessKeys = null;
    private AmazonS3 s3Client = null;

    @BeforeClass
    private void setUp() throws Exception {
        localPath = new File( S3TestBase.workDir + File.separator + TestTools
                .getClassName() );
        filePath =
                localPath + File.separator + "localFile_" + fileSize + ".txt";
        updatePath =
                localPath + File.separator + "localFile_" + updateSize + ".txt";
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath, fileSize );
        TestTools.LocalFile.createFile( updatePath, updateSize );

        CommLib.clearUser( userName );
        acessKeys = UserUtils.createUser( userName, roleName );
        s3Client = CommLib.buildS3Client( acessKeys[ 0 ], acessKeys[ 1 ] );
        s3Client.createBucket( bucketName );
    }

    @Test
    public void testUpdateAndGetBucket() throws Exception {
        UpdateObjectThread updateObject = new UpdateObjectThread();
        GetObjectThread getObject = new GetObjectThread();
        updateObject.start();
        getObject.start();

        if ( updateObject.isSuccess() ) {
            if ( getObject.isSuccess() ) {
                checkGetObject( bucketName, keyName );
            } else {
                AmazonS3Exception e = ( AmazonS3Exception ) ( getObject
                        .getExceptions().get( 0 ) );
                if ( !e.getErrorCode().equals( "NoSuchKey" ) ) {
                    Assert.fail(
                            "getObject fail:" + getObject.getErrorMsg() + "  e:"
                                    + e.getErrorCode() );
                }
            }
            checkUpdateObjectResult( bucketName, keyName );
        } else {
            Assert.fail( "Unexpected results! updateObjectError:" + updateObject
                    .getErrorMsg() + "getObjectError:" + getObject
                    .getErrorMsg() );
        }

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( runSuccess ) {
                UserUtils.deleteUser( userName );
                TestTools.LocalFile.removeFile( localPath );
            }
        } catch ( BaseException e ) {
            Assert.fail( "clean up failed:" + e.getMessage() );
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
        String downfileMd5 = ObjectUtils
                .getMd5OfObject( s3Client, localPath, bucketName, key );
        Assert.assertEquals( downfileMd5, TestTools.getMD5( updatePath ) );
    }

    private class UpdateObjectThread extends S3ThreadBase {
        @Override
        public void exec() throws Exception {
            AmazonS3 s3Client = CommLib
                    .buildS3Client( acessKeys[ 0 ], acessKeys[ 1 ] );
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
            AmazonS3 s3Client = CommLib
                    .buildS3Client( acessKeys[ 0 ], acessKeys[ 1 ] );
            try {
                S3Object object = s3Client.getObject( bucketName, keyName );
                objectLength = object.getObjectMetadata().getContentLength();
                S3ObjectInputStream s3is = object.getObjectContent();
                String downloadPath = TestTools.LocalFile
                        .initDownloadPath( localPath, TestTools.getMethodName(),
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
