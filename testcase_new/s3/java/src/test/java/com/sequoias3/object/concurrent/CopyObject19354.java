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
 * @Description seqDB-19354:并发复制对象和删除目标桶
 * @author wuyan
 * @Date 2019.09.20
 * @version 1.00
 */
public class CopyObject19354 extends S3TestBase {

    private boolean runSuccess = false;
    private String srcKeyName = "/SRC/bb%object19354";
    private String destKeyName = "/dest/bb/object19354";
    private String srcBucketName = "srcbucket19354";
    private String destBucketName = "destbucket19354";
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * 20;
    private File localPath = null;
    private String filePath = null;

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator + TestTools
                .getClassName() );
        filePath =
                localPath + File.separator + "localFile_" + fileSize + ".txt";
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath, fileSize );

        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, srcBucketName );
        CommLib.clearBucket( s3Client, destBucketName );
        s3Client.createBucket( srcBucketName );
        s3Client.createBucket( destBucketName );
        s3Client.putObject( srcBucketName, srcKeyName, new File( filePath ) );
    }

    @SuppressWarnings("deprecation")
    @Test
    public void testCopyObject() throws Exception {
        ThreadExecutor threadExec = new ThreadExecutor();
        CopyObject copyObject = new CopyObject( destKeyName );
        DeleteBucket deleteBucket = new DeleteBucket( destBucketName );
        threadExec.addWorker( copyObject );
        threadExec.addWorker( deleteBucket );
        threadExec.run();
        int deleteBucketErrCode = deleteBucket.getRetCode();
        int copyObjectErrCode = copyObject.getRetCode();
        if ( copyObjectErrCode == 0 ) {
            // copy object success.delete bucket fail,the errorcode 409(BucketNotEmpty)
            Assert.assertEquals( deleteBucketErrCode, 409 );
            checkObjectContent( destBucketName, destKeyName );
        } else {
            // delete bucket success,copy object fail,the errorCode:404(NoSuchBucket)
            //or the errorCode:200(源对象在copy过程中被删除，有可能报200错误)
            Assert.assertTrue( copyObjectErrCode == 404 ||
                    copyObjectErrCode == 200 );
            Assert.assertFalse( s3Client.doesBucketExist( destBucketName ) );
        }
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                CommLib.clearBucket( s3Client, srcBucketName );
                CommLib.clearBucket( s3Client, destBucketName );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            s3Client.shutdown();
        }
    }

    private void checkObjectContent( String bucketName, String keyName )
            throws Exception {
        String downfileMd5 = ObjectUtils
                .getMd5OfObject( s3Client, localPath, bucketName, keyName );
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
                s3Client1.deleteBucket( destBucketName );
                CopyObjectRequest request = new CopyObjectRequest(
                        srcBucketName, srcKeyName, destBucketName,
                        destKeyName );
                s3Client1.copyObject( request );
            } catch ( AmazonS3Exception e ) {
                int statusCode = e.getStatusCode();
                saveResult( statusCode, e );
                // 404:NoSuchBucket
                String errCode = e.getErrorCode();
                if ( !errCode.equals( "NoSuchBucket" ) ) {
                    throw e;
                }
            } finally {
                if ( s3Client1 != null ) {
                    s3Client1.shutdown();
                }
            }
        }
    }

    private class DeleteBucket extends ResultStore {
        private AmazonS3 s3Client2 = CommLib.buildS3Client();
        private String bucketName;

        private DeleteBucket( String bucketName ) {
            this.bucketName = bucketName;

        }

        @ExecuteOrder(step = 1)
        private void deleteBucket() throws Exception {
            // random waiting time is less than 100ms.run randomly to different concurrency results.
            int random = ( int ) ( Math.random() * 100 );
            Thread.sleep( random );
            try {
                s3Client2.deleteBucket( bucketName );
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
