package com.sequoias3.object.concurrent;

import com.amazonaws.services.s3.AmazonS3;
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
 * @Description seqDB-16491:concurrent update and delete the same object
 * @author wuyan
 * @Date 2019.1.8
 * @version 1.00
 */
public class UpdateAndDeleteSameObject16491 extends S3TestBase {
    private boolean runSuccess = false;
    private String keyName = "aa/bb/object16491";
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * 1024 * 4;
    private int updateSize = 1024 * 1024 * 3;
    private File localPath = null;
    private String filePath = null;
    private String updatePath = null;

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

        s3Client = CommLib.buildS3Client();
        ObjectUtils.clearOneObject( s3Client, S3TestBase.bucketName, keyName );
        s3Client.putObject( S3TestBase.bucketName, keyName,
                new File( filePath ) );
    }

    @Test
    public void testCreateBucket() throws Exception {
        UpdateObjectThread updateObjectThread = new UpdateObjectThread();
        DeleteObjectThread deleteObjectThread = new DeleteObjectThread();
        deleteObjectThread.start();
        updateObjectThread.start();

        Assert.assertTrue( updateObjectThread.isSuccess(),
                updateObjectThread.getErrorMsg() );
        Assert.assertTrue( deleteObjectThread.isSuccess(),
                deleteObjectThread.getErrorMsg() );

        checkUpdateAndDeleteObjectResult( S3TestBase.bucketName, keyName );

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( runSuccess ) {
                ObjectUtils.clearOneObject( s3Client, S3TestBase.bucketName,
                        keyName );
                TestTools.LocalFile.removeFile( localPath );
            }
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
            Assert.assertFalse( isExistObject, "the object must be deleted!" );
        }
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

    private class UpdateObjectThread extends S3ThreadBase {
        @Override
        public void exec() throws Exception {
            AmazonS3 s3Client = CommLib.buildS3Client();
            try {
                s3Client.putObject( S3TestBase.bucketName, keyName,
                        new File( updatePath ) );
            } finally {
                if ( s3Client != null ) {
                    s3Client.shutdown();
                }
            }
        }
    }
}
