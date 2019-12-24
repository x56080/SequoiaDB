package com.sequoias3.partupload.concurrent;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.ListMultipartUploadsRequest;
import com.amazonaws.services.s3.model.MultipartUploadListing;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.PartUploadUtils;
import org.springframework.util.LinkedMultiValueMap;
import org.springframework.util.MultiValueMap;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

/**
 * @Description seqDB-18770:并发查询分段上传列表 (test point b: the different condition)
 * @Author huangxiaoni
 * @Date 2019.07.26
 */

public class UploadPart18770B extends S3TestBase {
    private boolean runSuccess = false;
    private AmazonS3 s3Client;
    private String bucketName = "bucket18770b";
    private File localPath;
    private String filePath;
    private File file;
    private long fileSize = 5 * 1024 * 1024;
    private int maxPartNumber = 10;
    private String[] keys = { "atest18770b_0", "/dir1/test18770b_1",
            "/dir1/dir2/test18770b_2", "/dira/test18770b_3", "test18770b_4" };
    private List<String> uploadIdsOld = new ArrayList<>();
    private List<String> uploadIdsNew = new ArrayList<>();

    @BeforeClass
    private void setUp() throws IOException {
        this.initFile();
        s3Client = CommLib.buildS3Client();
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );
        this.initAndUploadPart();
    }

    @Test
    private void test() throws Exception {
        // list and check results
        ThreadExecutor threadExec = new ThreadExecutor();
        threadExec.addWorker( new ThreadListCondA() );
        threadExec.addWorker( new ThreadListCondB() );
        threadExec.run();

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                s3Client.deleteBucket( bucketName );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            s3Client.shutdown();
        }
    }

    private void initAndUploadPart() {
        // initPartUpload
        for ( String key : keys ) {
            String uploadId = PartUploadUtils
                    .initPartUpload( s3Client, bucketName, key );
            uploadIdsOld.add( uploadId );
        }
        // initPartUpload again
        for ( String key : keys ) {
            String uploadId = PartUploadUtils
                    .initPartUpload( s3Client, bucketName, key );
            uploadIdsNew.add( uploadId );
        }

        // uploadPart, multi part
        for ( int i = 0; i < keys.length; i++ ) {
            PartUploadUtils.partUpload( s3Client, bucketName, keys[ i ],
                    uploadIdsNew.get( i ), file, fileSize / maxPartNumber );
        }
    }

    private void initFile() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator + TestTools
                .getClassName() );
        filePath =
                localPath + File.separator + "localFile_" + fileSize + ".txt";
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath, fileSize );
        file = new File( filePath );
    }

    private class ThreadListCondA {
        private AmazonS3 s3;
        private MultipartUploadListing result;

        @ExecuteOrder(step = 1, desc = "connect s3")
        private void connectS3() {
            s3 = CommLib.buildS3Client();
        }

        @ExecuteOrder(step = 2, desc = "list")
        private void list() {
            ListMultipartUploadsRequest request = new ListMultipartUploadsRequest(
                    bucketName ).withPrefix( "/dir" ).withKeyMarker( keys[ 2 ] )
                    .withUploadIdMarker( uploadIdsOld.get( 2 ) );
            result = s3.listMultipartUploads( request );
        }

        @ExecuteOrder(step = 3, desc = "check results")
        private void checkResults() {
            List<String> expCommonPrefixes = new ArrayList<>();
            MultiValueMap<String, String> expUploads = new LinkedMultiValueMap<String, String>();
            expUploads.add( keys[ 2 ], uploadIdsNew.get( 2 ) );
            expUploads.add( keys[ 1 ], uploadIdsOld.get( 1 ) );
            expUploads.add( keys[ 1 ], uploadIdsNew.get( 1 ) );
            expUploads.add( keys[ 3 ], uploadIdsOld.get( 3 ) );
            expUploads.add( keys[ 3 ], uploadIdsNew.get( 3 ) );
            PartUploadUtils.checkListMultipartUploadsResults( result,
                    expCommonPrefixes, expUploads );
        }

        @ExecuteOrder(step = 4, desc = "shutdown s3")
        private void shutdownS3() {
            s3.shutdown();
        }
    }

    private class ThreadListCondB {
        private AmazonS3 s3;
        private MultipartUploadListing result;

        @ExecuteOrder(step = 1, desc = "connect s3")
        private void connectS3() {
            s3 = CommLib.buildS3Client();
        }

        @ExecuteOrder(step = 2, desc = "list")
        private void list() {
            ListMultipartUploadsRequest request = new ListMultipartUploadsRequest(
                    bucketName ).withPrefix( "/dir" ).withMaxUploads( 3 );
            result = s3.listMultipartUploads( request );
        }

        @ExecuteOrder(step = 3, desc = "check results")
        private void checkResults() {
            List<String> expCommonPrefixes = new ArrayList<>();
            MultiValueMap<String, String> expUploads = new LinkedMultiValueMap<String, String>();
            expUploads.add( keys[ 2 ], uploadIdsOld.get( 2 ) );
            expUploads.add( keys[ 2 ], uploadIdsNew.get( 2 ) );
            expUploads.add( keys[ 1 ], uploadIdsOld.get( 1 ) );
            PartUploadUtils.checkListMultipartUploadsResults( result,
                    expCommonPrefixes, expUploads );
        }

        @ExecuteOrder(step = 4, desc = "shutdown s3")
        private void shutdownS3() {
            s3.shutdown();
        }
    }
}