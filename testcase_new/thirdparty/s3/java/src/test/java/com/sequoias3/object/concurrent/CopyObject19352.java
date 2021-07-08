package com.sequoias3.object.concurrent;

import java.io.File;
import java.io.IOException;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.CopyObjectRequest;
import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.ObjectUtils;

/**
 * @Description seqDB-19352:并发复制对象和删除源对象
 * @author wuyan
 * @Date 2019.09.20
 * @version 1.00
 */
public class CopyObject19352 extends S3TestBase {
    private boolean runSuccess = false;
    private String srcKeyName = "/SRC/bb%object19352";
    private String destKeyName = "/dest/bb/object19352";
    private String bucketName = "bucket19352";
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * 1024 * 20;
    private File localPath = null;
    private String filePath = null;

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator
                + TestTools.getClassName() );
        filePath = localPath + File.separator + "localFile_" + fileSize
                + ".txt";
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath, fileSize );

        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
        s3Client.putObject( bucketName, srcKeyName, new File( filePath ) );
    }

    @Test
    public void testCopyObject() throws Exception {

        ThreadExecutor threadExec = new ThreadExecutor();
        CopyObject copyObject = new CopyObject( destKeyName );
        DeleteObject deleteObject = new DeleteObject( srcKeyName );
        threadExec.addWorker( copyObject );
        threadExec.addWorker( deleteObject );
        threadExec.run();
        int deleteObjectErrCode = deleteObject.getRetCode();
        int copyObjectErrCode = copyObject.getRetCode();
        if ( copyObjectErrCode == 0 ) {
            // delete object success,copy object success.
            Assert.assertEquals( deleteObjectErrCode, 0 );
            checkObjectContent( destKeyName );
            Assert.assertFalse(
                    s3Client.doesObjectExist( bucketName, srcKeyName ) );
        } else {
            // delete object success,copy object fail,the
            // errorCode:404(NoSuchKey)
            // or the errorCode:200(源对象在copy过程中被删除，有可能报200错误)
            Assert.assertTrue(
                    copyObjectErrCode == 404 || copyObjectErrCode == 200 );
            Assert.assertFalse(
                    s3Client.doesObjectExist( bucketName, srcKeyName ) );
            Assert.assertFalse(
                    s3Client.doesObjectExist( bucketName, destKeyName ) );
        }

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                CommLib.clearBucket( s3Client, bucketName );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            s3Client.shutdown();
        }
    }

    private void checkObjectContent( String destKeyName ) throws Exception {
        String downfileMd5 = ObjectUtils.getMd5OfObject( s3Client, localPath,
                bucketName, destKeyName );
        Assert.assertEquals( downfileMd5, TestTools.getMD5( filePath ) );
    }

    private class CopyObject extends ResultStore {
        private AmazonS3 s3Client1 = CommLib.buildS3Client();
        private String destKeyName;

        private CopyObject( String destKeyName ) {
            this.destKeyName = destKeyName;

        }

        @ExecuteOrder(step = 1)
        private void copyObject() throws Exception {
            try {
                CopyObjectRequest request = new CopyObjectRequest( bucketName,
                        srcKeyName, bucketName, destKeyName );
                s3Client1.copyObject( request );
            } catch ( AmazonS3Exception e ) {
                int statusCode = e.getStatusCode();
                saveResult( statusCode, e );
            } finally {
                if ( s3Client1 != null ) {
                    s3Client1.shutdown();
                }
            }
        }
    }

    private class DeleteObject extends ResultStore {
        private AmazonS3 s3Client2 = CommLib.buildS3Client();
        private String keyName;

        private DeleteObject( String keyName ) {
            this.keyName = keyName;

        }

        @ExecuteOrder(step = 1)
        private void deleteObject() throws Exception {
            try {
                s3Client2.deleteObject( bucketName, keyName );
            } catch ( AmazonS3Exception e ) {
                int statusCode = e.getStatusCode();
                saveResult( statusCode, e );
            } finally {
                if ( s3Client2 != null ) {
                    s3Client2.shutdown();
                }
            }
        }
    }

}
