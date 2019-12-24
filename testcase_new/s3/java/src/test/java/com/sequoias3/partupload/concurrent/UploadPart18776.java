package com.sequoias3.partupload.concurrent;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AbortMultipartUploadRequest;
import com.amazonaws.services.s3.model.BucketVersioningConfiguration;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.ListMultipartUploadsRequest;
import com.amazonaws.services.s3.model.MultipartUploadListing;
import com.amazonaws.services.s3.model.PartETag;
import com.amazonaws.services.s3.model.UploadPartRequest;
import com.amazonaws.services.s3.model.UploadPartResult;
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
 * @Description seqDB-18776:相同对象不同uploadId并发完成分段上传和终止分段上传
 * @Author huangxiaoni
 * @Date 2019.07.26
 */

public class UploadPart18776 extends S3TestBase {
    private boolean runSuccess = false;
    private File localPath;
    private String filePath;
    private String expFilePath;
    private File file;
    private int fileSize = 60 * 1024 * 1024;

    private AmazonS3 s3Client;
    private String bucketName = "bucket18776";
    private String key = "obj18776";
    private int partsNum = 10;
    private List<PartETag> partETags = new ArrayList<>();

    @BeforeClass
    private void setUp() throws IOException {
        this.initFile();
        s3Client = CommLib.buildS3Client();
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );
        CommLib.setBucketVersioning( s3Client, bucketName,
                BucketVersioningConfiguration.ENABLED );
    }

    @Test
    private void test() throws Exception {
        int partSize = fileSize / partsNum;

        // init upload part
        String uploadId1 = PartUploadUtils
                .initPartUpload( s3Client, bucketName, key );
        String uploadId2 = PartUploadUtils
                .initPartUpload( s3Client, bucketName, key );

        // upload part
        for ( int i = 0; i < partsNum; i++ ) {
            int fileOffset = partSize * i;
            int partNumber = i + 1;
            String uploadId;
            if ( i < partsNum / 2 ) {
                uploadId = uploadId1;
            } else {
                uploadId = uploadId2;
            }
            this.uploadPart( uploadId, fileOffset, partNumber, partSize );
        }

        // complete and abort upload
        ThreadExecutor threadExec = new ThreadExecutor();
        threadExec.addWorker( new ThreadCompleteUpload( uploadId1,
                partETags.subList( 0, partsNum / 2 ) ) );
        threadExec.addWorker( new ThreadAbortUpload( uploadId2 ) );
        threadExec.run();

        // check complete upload part
        int len = partsNum / 2 * partSize;
        TestTools.LocalFile.readFile( filePath, 0, len, expFilePath );
        String downfileMd5 = ObjectUtils
                .getMd5OfObject( s3Client, localPath, bucketName, key );
        Assert.assertEquals( downfileMd5, TestTools.getMD5( expFilePath ) );

        // check abort upload part
        ListMultipartUploadsRequest request = new ListMultipartUploadsRequest(
                bucketName );
        MultipartUploadListing result = s3Client
                .listMultipartUploads( request );
        List<String> expCommonPrefixes = new ArrayList<>();
        MultiValueMap<String, String> expUploads = new LinkedMultiValueMap<String, String>();
        PartUploadUtils
                .checkListMultipartUploadsResults( result, expCommonPrefixes,
                        expUploads );

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
        localPath = new File( S3TestBase.workDir + File.separator + TestTools
                .getClassName() );
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        filePath = this.createFile( fileSize );
        expFilePath = this.createFile( 0 );
        file = new File( filePath );
    }

    private String createFile( int fileSize ) throws IOException {
        String filePath =
                localPath + File.separator + "localFile_" + fileSize + ".txt";
        TestTools.LocalFile.createFile( filePath, fileSize );
        return filePath;
    }

    private class ThreadCompleteUpload {
        private String uploadId;
        private List<PartETag> eTags;

        public ThreadCompleteUpload( String uploadId, List<PartETag> eTags ) {
            this.uploadId = uploadId;
            this.eTags = eTags;
        }

        @ExecuteOrder(step = 1)
        private void uploadPart() {
            AmazonS3 s3 = null;
            try {
                s3 = CommLib.buildS3Client();
                PartUploadUtils
                        .completeMultipartUpload( s3, bucketName, key, uploadId,
                                eTags );
            } finally {
                if ( s3 != null ) {
                    s3.shutdown();
                }
            }
        }
    }

    private class ThreadAbortUpload {
        private String uploadId;

        public ThreadAbortUpload( String uploadId ) {
            this.uploadId = uploadId;
        }

        @ExecuteOrder(step = 1)
        private void uploadPart() {
            AmazonS3 s3 = null;
            try {
                s3 = CommLib.buildS3Client();
                s3Client.abortMultipartUpload(
                        new AbortMultipartUploadRequest( bucketName, key,
                                uploadId ) );
            } finally {
                if ( s3 != null ) {
                    s3.shutdown();
                }
            }
        }
    }
}