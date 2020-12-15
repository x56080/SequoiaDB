package com.sequoias3.partupload.concurrent;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AbortMultipartUploadRequest;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.PartUploadUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.io.IOException;

/**
 * @Description seqDB-18767: the key upload multiple parts and
 *              AbortMultipartUpload concurrently by the same key.
 * @author wuyan
 * @Date 2019.08.06
 * @version 1.00
 */
public class AbortMultipartUploadBySameKey18767 extends S3TestBase {
    private boolean runSuccess = false;
    private String keyName = "/aa/object18767";
    private String bucketName = "bucket18767";
    private AmazonS3 s3Client = null;
    private File localPath = null;
    private String[] filePaths = new String[ 5 ];
    private int[] fileSizes = { 1024 * 1024 * 40, 1024 * 1024 * 29,
            1024 * 1024 * 30, 1024 * 1024 * 10, 1024 * 1024 * 30 };

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator
                + TestTools.getClassName() );
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        for ( int i = 0; i < fileSizes.length; i++ ) {
            String filePath = localPath + File.separator + "localFile_"
                    + fileSizes[ i ] + ".txt";
            TestTools.LocalFile.createFile( filePath, fileSizes[ i ] );
            filePaths[ i ] = filePath;
        }

        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );
    }

    @Test
    public void abortMultipartUpload() throws Exception {
        ThreadExecutor threadExec = new ThreadExecutor();
        int[] partSizes = { 1024 * 1024 * 5, 1024 * 1024 * 6, 1024 * 1024 * 6,
                1024 * 1024 * 5, 1024 * 1024 * 10 };
        for ( int i = 0; i < filePaths.length; i++ ) {
            String filePath = filePaths[ i ];
            int partSize = partSizes[ i ];
            threadExec.addWorker(
                    new AbortMultipartUpload( filePath, partSize ) );
        }
        threadExec.run();
        checkResult();

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

    private void checkResult() {
        // get key is not exist.
        try {
            s3Client.getObject( bucketName, keyName );
            Assert.fail( "get not exist key must be fail !" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "NoSuchKey" );
        }
    }

    private class AbortMultipartUpload extends ResultStore {
        private String filePath;
        private String uploadId;
        private int partSize;
        private File file = null;
        private AmazonS3 s3Client1 = CommLib.buildS3Client();

        private AbortMultipartUpload( String filePath, int partSize ) {
            this.filePath = filePath;
            this.partSize = partSize;
        }

        @ExecuteOrder(step = 1)
        private void initPartUpload() {
            uploadId = PartUploadUtils.initPartUpload( s3Client1, bucketName,
                    keyName );
        }

        @ExecuteOrder(step = 2)
        private void partUpload() {
            file = new File( filePath );
            PartUploadUtils.partUpload( s3Client1, bucketName, keyName,
                    uploadId, file, partSize );
        }

        @ExecuteOrder(step = 3)
        private void completeMultipartUpload() throws IOException {
            try {
                AbortMultipartUploadRequest request = new AbortMultipartUploadRequest(
                        bucketName, keyName, uploadId );
                s3Client.abortMultipartUpload( request );
            } finally {
                if ( s3Client1 != null ) {
                    s3Client1.shutdown();
                }
            }
        }
    }
}
