package com.sequoias3.partupload.concurrent;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AbortMultipartUploadRequest;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.ListMultipartUploadsRequest;
import com.amazonaws.services.s3.model.MultipartUploadListing;
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
import org.springframework.util.LinkedMultiValueMap;
import org.springframework.util.MultiValueMap;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

/**
 * @Description seqDB-18777:相同对象相同uploadId并发完成分段上传和终止分段上传
 * @Author huangxiaoni
 * @Date 2019.07.26
 */

public class UploadPart18777 extends S3TestBase {
    private boolean runSuccess = false;
    private File localPath;
    private String filePath;
    private File file;
    private int fileSize = 60 * 1024 * 1024;

    private AmazonS3 s3Client;
    private String bucketName = "bucket18777";
    private String key = "obj18777";
    private int partsNum = 10;
    private String uploadId;
    private List< PartETag > partETags = new ArrayList<>();

    @BeforeClass
    private void setUp() throws IOException {
        this.initFile();
        s3Client = CommLib.buildS3Client();
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );
    }

    @Test
    private void test() throws Exception {
        // init upload part
        uploadId = PartUploadUtils.initPartUpload( s3Client, bucketName, key );

        // upload part
        int partSize = fileSize / partsNum;
        for ( int i = 0; i < partsNum; i++ ) {
            int fileOffset = partSize * i;
            int partNumber = i + 1;
            this.uploadPart( uploadId, fileOffset, partNumber, partSize );
        }

        // complete and abort upload
        ThreadExecutor threadExec = new ThreadExecutor();
        ThreadCompleteUpload threadCompUpload = new ThreadCompleteUpload();
        threadExec.addWorker( threadCompUpload );
        threadExec.addWorker( new ThreadAbortUpload() );
        threadExec.run();

        // check results
        if ( threadCompUpload.getRetCode() == 0 ) {
            // complete upload success
            String downfileMd5 = ObjectUtils.getMd5OfObject( s3Client,
                    localPath, bucketName, key );
            Assert.assertEquals( downfileMd5, TestTools.getMD5( filePath ) );
        } else {
            // abort upload success
            ListMultipartUploadsRequest request = new ListMultipartUploadsRequest(
                    bucketName );
            MultipartUploadListing result = s3Client
                    .listMultipartUploads( request );
            List< String > expCommonPrefixes = new ArrayList<>();
            MultiValueMap< String, String > expUploads = new LinkedMultiValueMap< String, String >();
            PartUploadUtils.checkListMultipartUploadsResults( result,
                    expCommonPrefixes, expUploads );
            try {
                s3Client.getObject( bucketName, key );
            } catch ( AmazonS3Exception e ) {
                if ( !e.getErrorCode().equals( "NoSuchKey" ) ) {
                    throw e;
                }
            }
        }

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                s3Client.deleteObject( bucketName, key );
                s3Client.deleteBucket( bucketName );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            s3Client.shutdown();
        }
    }

    private void uploadPart( String uploadId, int fileOffset, int partNumber,
            int partSize ) {
        UploadPartRequest partRequest = new UploadPartRequest()
                .withBucketName( bucketName ).withKey( key ).withFile( file )
                .withUploadId( uploadId ).withFileOffset( fileOffset )
                .withPartNumber( partNumber ).withPartSize( partSize );
        UploadPartResult partResult = s3Client.uploadPart( partRequest );
        partETags.add( partResult.getPartETag() );
    }

    private void initFile() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator
                + TestTools.getClassName() );
        filePath = localPath + File.separator + "localFile_" + fileSize
                + ".txt";
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath, fileSize );
        file = new File( filePath );
    }

    private class ThreadCompleteUpload extends ResultStore {
        @ExecuteOrder(step = 1)
        private void uploadPart() {
            AmazonS3 s3 = null;
            try {
                s3 = CommLib.buildS3Client();
                PartUploadUtils.completeMultipartUpload( s3, bucketName, key,
                        uploadId, partETags );
            } catch ( AmazonS3Exception e ) {
                if ( !e.getErrorCode().equals( "NoSuchUpload" ) ) {
                    throw e;
                }
                saveResult( -1, e );
            } finally {
                if ( s3 != null ) {
                    s3.shutdown();
                }
            }
        }
    }

    private class ThreadAbortUpload {
        @ExecuteOrder(step = 1)
        private void uploadPart() {
            AmazonS3 s3 = null;
            try {
                s3 = CommLib.buildS3Client();
                s3.abortMultipartUpload( new AbortMultipartUploadRequest(
                        bucketName, key, uploadId ) );
            } catch ( AmazonS3Exception e ) {
                if ( !e.getErrorCode().equals( "NoSuchUpload" ) ) {
                    throw e;
                }
            } finally {
                if ( s3 != null ) {
                    s3.shutdown();
                }
            }
        }
    }
}