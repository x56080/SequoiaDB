package com.sequoias3.partupload.concurrent;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.PartETag;
import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import com.sequoias3.testcommon.s3utils.PartUploadUtils;
import org.springframework.util.LinkedMultiValueMap;
import org.springframework.util.MultiValueMap;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.io.IOException;
import java.util.List;

/**
 * @Description seqDB-18765: the key upload multiple parts and
 *              completeMultipartUpload concurrently by the same uploadId.
 * @author wuyan
 * @Date 2019.08.06
 * @version 1.00
 */
public class CompleteMultipartUploadBySameUploadId18765 extends S3TestBase {
    private boolean runSuccess = false;
    private String keyName = "/aa/object18765";
    private String bucketName = "bucket18765";
    private AmazonS3 s3Client = null;
    private File localPath = null;
    private String[] filePaths = new String[ 5 ];
    private int[] fileSizes = { 1024 * 1024 * 50, 1024 * 1024 * 29,
            1024 * 1024 * 30, 1024 * 1024 * 10, 1024 * 1024 * 40 };
    private MultiValueMap< Long, String > uploadFileSizeAndMd5 = new LinkedMultiValueMap< Long, String >();
    private int successNum = 0;

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
    public void uploadParts() throws Exception {
        String uploadId = PartUploadUtils.initPartUpload( s3Client, bucketName,
                keyName );
        ThreadExecutor threadExec = new ThreadExecutor();
        int[] partSizes = { 1024 * 1024 * 5, 1024 * 1024 * 6, 1024 * 1024 * 6,
                1024 * 1024 * 5, 1024 * 1024 * 10 };
        for ( int i = 0; i < filePaths.length; i++ ) {
            String filePath = filePaths[ i ];
            int partSize = partSizes[ i ];
            threadExec.addWorker(
                    new PartUpload( uploadId, filePath, partSize ) );
        }
        threadExec.run();

        // check the upload file
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

    private void checkResult() throws Exception {
        // only one completeMultipartUpload success
        int expSuccessNum = 1;
        Assert.assertEquals( successNum, expSuccessNum );
        // get the upload object to check content by md5
        String downfileMd5 = ObjectUtils.getMd5OfObject( s3Client, localPath,
                bucketName, keyName );
        long size = ( long ) uploadFileSizeAndMd5.keySet().toArray()[ 0 ];
        String expFileMd5 = uploadFileSizeAndMd5.get( size ).get( 0 );
        Assert.assertEquals( downfileMd5, expFileMd5,
                "the object size is :" + size );
    }

    private class PartUpload extends ResultStore {
        private String filePath;
        private String uploadId;
        private int partSize;
        private List< PartETag > partEtags;
        private File file = null;
        private AmazonS3 s3Client1 = CommLib.buildS3Client();

        private PartUpload( String uploadId, String filePath, int partSize ) {
            this.uploadId = uploadId;
            this.filePath = filePath;
            this.partSize = partSize;
        }

        @ExecuteOrder(step = 1)
        private void partUpload() {
            file = new File( filePath );
            partEtags = PartUploadUtils.partUpload( s3Client1, bucketName,
                    keyName, uploadId, file, partSize );
        }

        @ExecuteOrder(step = 2)
        private void completeMultipartUpload() throws IOException {
            try {
                PartUploadUtils.completeMultipartUpload( s3Client1, bucketName,
                        keyName, uploadId, partEtags );
                successNum++;
                String fileMd5 = TestTools.getMD5( filePath );
                uploadFileSizeAndMd5.add( file.length(), fileMd5 );
            } catch ( AmazonS3Exception e ) {
                int errCode = e.getStatusCode();
                // 400: InvalidPart,404:NoSuchUpload
                if ( errCode != 400 && errCode != 404 ) {
                    throw e;
                }
            } finally {
                if ( s3Client1 != null ) {
                    s3Client1.shutdown();
                }
            }
        }
    }
}
