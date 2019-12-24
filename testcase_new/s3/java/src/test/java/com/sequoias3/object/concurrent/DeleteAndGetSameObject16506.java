package com.sequoias3.object.concurrent;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
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
 * test content: 开启版本控制，并发删除和获取对象（指定版本） testlink-case: seqDB-16506
 *
 * @author wangkexin
 * @Date 2019.01.04
 * @version 1.00
 */
public class DeleteAndGetSameObject16506 extends S3TestBase {
    private boolean runSuccess = false;
    private String userName = "user16506";
    private String bucketName = "bucket16506";
    private String keyName = "key16506";
    private String roleName = "normal";
    private String content = "testContent16506";
    private String deleteVersionId = "1";
    private File localPath = null;
    private String[] acessKeys = null;
    private AmazonS3 s3Client = null;

    @BeforeClass
    private void setUp() throws Exception {
        localPath = new File( S3TestBase.workDir + File.separator + TestTools
                .getClassName() );
        CommLib.clearUser( userName );
        acessKeys = UserUtils.createUser( userName, roleName );
        s3Client = CommLib.buildS3Client( acessKeys[ 0 ], acessKeys[ 1 ] );
        s3Client.createBucket( bucketName );
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );
        s3Client.putObject( bucketName, keyName, content + "v0" );
        s3Client.putObject( bucketName, keyName, content + "v1" );
        s3Client.putObject( bucketName, keyName, content + "v2" );
    }

    @Test
    public void testDeleteAndGetObject() throws Exception {
        GetObjectThread getObject = new GetObjectThread();
        DeleteObjectThread deleteObject = new DeleteObjectThread();
        getObject.start();
        deleteObject.start();

        Assert.assertTrue( deleteObject.isSuccess(),
                deleteObject.getErrorMsg() );
        Assert.assertTrue( getObject.isSuccess(), getObject.getErrorMsg() );
        Assert.assertTrue( s3Client.doesObjectExist( bucketName, keyName ) );

        try {
            s3Client.getObject( new GetObjectRequest( bucketName, keyName,
                    deleteVersionId ) );
            Assert.fail( "exp fail but found success!" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "NoSuchVersion" );
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

    private class DeleteObjectThread extends S3ThreadBase {
        @Override
        public void exec() throws Exception {
            AmazonS3 s3Client = CommLib
                    .buildS3Client( acessKeys[ 0 ], acessKeys[ 1 ] );
            try {
                s3Client.deleteVersion( bucketName, keyName, deleteVersionId );
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
                S3Object object = s3Client.getObject(
                        new GetObjectRequest( bucketName, keyName,
                                deleteVersionId ) );
                S3ObjectInputStream s3is = object.getObjectContent();
                String downloadPath = TestTools.LocalFile
                        .initDownloadPath( localPath, TestTools.getMethodName(),
                                Thread.currentThread().getId() );
                ObjectUtils.inputStream2File( s3is, downloadPath );
                s3is.close();
                String getObjectMd5 = TestTools.getMD5( downloadPath );
                Assert.assertEquals( getObjectMd5,
                        TestTools.getMD5( ( content + "v1" ).getBytes() ),
                        "md5 is wrong!" );
                Assert.assertEquals( object.getObjectMetadata().getVersionId(),
                        deleteVersionId );
            } catch ( AmazonS3Exception e ) {
                Assert.assertEquals( e.getErrorCode(), "NoSuchVersion" );
            } finally {
                if ( s3Client != null ) {
                    s3Client.shutdown();
                }
            }
        }
    }
}
