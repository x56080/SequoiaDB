package com.sequoias3.object.concurrent;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.ObjectMetadata;
import com.amazonaws.services.s3.model.S3Object;
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
 * @Description seqDB-16492:concurrent get and delete the same object
 * @author wuyan
 * @Date 2019.1.8
 * @version 1.00
 */
public class DeleteAndGetSameObject16492 extends S3TestBase {
    private boolean runSuccess = false;
    private String keyName = "/dir-1/bb/object16492";
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * 1024 * 4;
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

        s3Client = CommLib.buildS3Client();
        ObjectUtils.clearOneObject( s3Client, S3TestBase.bucketName, keyName );
        s3Client.putObject( S3TestBase.bucketName, keyName,
                new File( filePath ) );
    }

    @Test
    public void testCreateBucket() throws Exception {
        GetObjectThread getObjectThread = new GetObjectThread();
        DeleteObjectThread deleteObjectThread = new DeleteObjectThread();
        deleteObjectThread.start();
        getObjectThread.start();

        if ( deleteObjectThread.isSuccess() ) {
            if ( !getObjectThread.isSuccess() ) {
                AmazonS3Exception e = ( AmazonS3Exception ) ( getObjectThread
                        .getExceptions().get( 0 ) );
                if ( !e.getErrorCode().equals( "NoSuchKey" ) ) {
                    Assert.fail(
                            "getObject fail:" + getObjectThread.getErrorMsg()
                                    + "  e:" + e.getErrorCode() );
                }
            }
            checkDeleteObjectResult( S3TestBase.bucketName, keyName );
        } else {
            Assert.fail( "Unexpected results! deleteObjectError:"
                    + deleteObjectThread.getErrorMsg() + "getObjectError:"
                    + getObjectThread.getErrorMsg() );
        }

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( runSuccess ) {
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }

    private void checkDeleteObjectResult( String bucketName, String key )
            throws Exception {
        boolean isExistObject = s3Client.doesObjectExist( bucketName, key );
        Assert.assertFalse( isExistObject, "the object must be deleted!" );
    }

    private class DeleteObjectThread extends S3ThreadBase {
        @Override
        public void exec() throws Exception {
            AmazonS3 s3Client = CommLib.buildS3Client();
            try {
                s3Client.deleteObject( S3TestBase.bucketName, keyName );
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
                S3Object object = s3Client
                        .getObject( S3TestBase.bucketName, keyName );
                ObjectMetadata metadata = object.getObjectMetadata();
                String etag = metadata.getETag();
                Assert.assertEquals( etag, TestTools.getMD5( filePath ) );
                String downfileMd5 = ObjectUtils
                        .getMd5OfObject( s3Client, localPath, bucketName,
                                keyName );
                Assert.assertEquals( downfileMd5,
                        TestTools.getMD5( filePath ) );
            } finally {
                if ( s3Client != null ) {
                    s3Client.shutdown();
                }
            }
        }
    }

}
