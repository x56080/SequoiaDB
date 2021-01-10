package com.sequoias3.partupload.concurrent;

import java.io.File;
import java.io.IOException;
import java.util.List;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.concurrent.atomic.AtomicInteger;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.PartETag;
import com.amazonaws.services.s3.model.UploadPartRequest;
import com.amazonaws.services.s3.model.UploadPartResult;
import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import com.sequoias3.testcommon.s3utils.PartUploadUtils;

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
    private String filePath = null;
    private int fileSize = 1024 * 1024 * 50;
    private int partSize = 1024 * 1024 * 5;
    private String uploadId;
    private List< PartETag > eTagList = new CopyOnWriteArrayList<>();
    private AtomicInteger successNum = new AtomicInteger( 0 );

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator
                + TestTools.getClassName() );
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        filePath = localPath + File.separator + "localFile_" + fileSize
                + ".txt";
        TestTools.LocalFile.createFile( filePath, fileSize );
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );
    }

    @Test
    public void uploadParts() throws Exception {
        uploadId = PartUploadUtils.initPartUpload( s3Client, bucketName,
                keyName );
        ThreadExecutor threadExec1 = new ThreadExecutor();
        // upload part
        int fileOffset = 0;
        for ( int i = 1; fileOffset < fileSize; i++ ) {
            long eachPartSize = Math.min( partSize, fileSize - fileOffset );
            threadExec1
                    .addWorker( new PartUpload( i, fileOffset, eachPartSize ) );
            fileOffset += partSize;
        }
        threadExec1.run();

        // complete upload
        ThreadExecutor threadExec2 = new ThreadExecutor();
        for ( int i = 0; i < 5; i++ ) {
            threadExec2.addWorker( new CompletePartUpload() );
        }
        threadExec2.run();

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
        Assert.assertEquals( successNum.get(), expSuccessNum );
        // get the upload object to check content by md5
        String downfileMd5 = ObjectUtils.getMd5OfObject( s3Client, localPath,
                bucketName, keyName );
        String expFileMd5 = TestTools.getMD5( filePath );
        Assert.assertEquals( downfileMd5, expFileMd5,
                "the filePath is :" + filePath );
    }

    private class CompletePartUpload extends ResultStore {
        private AmazonS3 s3Client1 = CommLib.buildS3Client();

        @ExecuteOrder(step = 1)
        private void completeMultipartUpload() throws IOException {
            try {
                PartUploadUtils.completeMultipartUpload( s3Client1, bucketName,
                        keyName, uploadId, eTagList );
                successNum.incrementAndGet();
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

    private class PartUpload {
        private int partNum;
        private long fileOffset;
        private long partSize;

        public PartUpload( int partNum, long fileOffset, long partSize ) {
            this.partNum = partNum;
            this.fileOffset = fileOffset;
            this.partSize = partSize;
        }

        @ExecuteOrder(step = 1)
        private void partUpload() {
            File file = new File( filePath );
            UploadPartRequest partRequest = new UploadPartRequest()
                    .withFile( file ).withFileOffset( fileOffset )
                    .withPartNumber( partNum ).withPartSize( partSize )
                    .withBucketName( bucketName ).withKey( keyName )
                    .withUploadId( uploadId );
            UploadPartResult uploadPartResult = s3Client
                    .uploadPart( partRequest );
            eTagList.add( uploadPartResult.getPartETag() );
        }
    }
}
