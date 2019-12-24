package com.sequoias3.object.concurrent;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.GetObjectRequest;
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
 * test content: 开启版本控制，并发更新和删除相同对象 testlink-case: seqDB-16504
 *
 * @author wangkexin
 * @Date 2019.01.04
 * @version 1.00
 */
public class UpdateAndDelectSameObject16504 extends S3TestBase {
    private boolean runSuccess = false;
    private String userName = "user16504";
    private String bucketName = "bucket16504";
    private String keyName = "key16504";
    private String roleName = "normal";
    private int fileSize = 1024 * 1024 * 4;
    private int updateSize = 1024 * 1024 * 3;
    private File localPath = null;
    private String filePath = null;
    private String updatePath = null;
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
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );
        s3Client.putObject( bucketName, keyName, new File( filePath ) );
    }

    @Test
    public void testUpdateAndDeleteObject() throws Exception {
        UpdateObjectThread updateObject = new UpdateObjectThread();
        DeleteObjectThread deleteObject = new DeleteObjectThread();
        updateObject.start();
        deleteObject.start();

        Assert.assertTrue( updateObject.isSuccess(),
                updateObject.getErrorMsg() );
        Assert.assertTrue( deleteObject.isSuccess(),
                deleteObject.getErrorMsg() );

        checkUpdateAndDeleteObjectResult( bucketName, keyName );
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

    private void checkUpdateAndDeleteObjectResult( String bucketName,
            String key ) throws Exception {
        boolean isExistObject = s3Client.doesObjectExist( bucketName, key );
        if ( isExistObject ) {
            String downfileMd5 = ObjectUtils
                    .getMd5OfObject( s3Client, localPath, bucketName, keyName );
            Assert.assertEquals( downfileMd5, TestTools.getMD5( updatePath ) );
        } else {
            S3Object object = s3Client.getObject(
                    new GetObjectRequest( bucketName, keyName, "1" ) );
            S3ObjectInputStream s3is = object.getObjectContent();
            String downloadPath = TestTools.LocalFile
                    .initDownloadPath( localPath, TestTools.getMethodName(),
                            Thread.currentThread().getId() );
            ObjectUtils.inputStream2File( s3is, downloadPath );
            s3is.close();
            String getObjectMd5 = TestTools.getMD5( downloadPath );
            Assert.assertEquals( getObjectMd5, TestTools.getMD5( filePath ) );
        }
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

    private class DeleteObjectThread extends S3ThreadBase {
        @Override
        public void exec() throws Exception {
            AmazonS3 s3Client = CommLib
                    .buildS3Client( acessKeys[ 0 ], acessKeys[ 1 ] );
            try {
                Thread.sleep( 30 );
                s3Client.deleteObject( bucketName, keyName );
            } finally {
                if ( s3Client != null ) {
                    s3Client.shutdown();
                }
            }
        }
    }
}
