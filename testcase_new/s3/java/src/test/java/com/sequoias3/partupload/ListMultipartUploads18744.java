package com.sequoias3.partupload;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.ListMultipartUploadsRequest;
import com.amazonaws.services.s3.model.MultipartUploadListing;
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
 * @Description seqDB-18744:带Key-marker查询桶分段上传列表
 * @Author huangxiaoni
 * @Date 2019.07.26
 */

public class ListMultipartUploads18744 extends S3TestBase {
    private int runSuccessNum = 0;
    private int expRunSuccessNum = 5;
    private AmazonS3 s3Client;
    private String bucketName = "bucket18744";
    private File localPath;
    private String filePath;
    private File file;
    private long fileSize = 10 * 1024 * 1024;
    private int maxPartNumber = 2;
    private String[] keys = { "/aa/bb/test18744_1", "/aa/bb/test18744_2",
            "test18744_3", "test18744_4" };
    private List<String> uploadIds = new ArrayList<>();

    private List<String> expCommonPrefixes = new ArrayList<>();
    private MultiValueMap<String, String> expUploads = new LinkedMultiValueMap<String, String>();

    @BeforeClass
    private void setUp() throws IOException {
        this.initFile();
        s3Client = CommLib.buildS3Client();
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );

        // uploadPart
        for ( String key : keys ) {
            String uploadId = PartUploadUtils
                    .initPartUpload( s3Client, bucketName, key );
            PartUploadUtils
                    .partUpload( s3Client, bucketName, key, uploadId, file,
                            fileSize / maxPartNumber );
            uploadIds.add( uploadId );
        }
    }

    @Test
    private void test_middleRecs() throws Exception {
        ListMultipartUploadsRequest request = new ListMultipartUploadsRequest(
                bucketName ).withKeyMarker( keys[ 1 ] );
        MultipartUploadListing result = s3Client
                .listMultipartUploads( request );
        expUploads.clear();
        expUploads.add( keys[ 2 ], uploadIds.get( 2 ) );
        expUploads.add( keys[ 3 ], uploadIds.get( 3 ) );
        PartUploadUtils
                .checkListMultipartUploadsResults( result, expCommonPrefixes,
                        expUploads );
        runSuccessNum++;
    }

    @Test
    private void test_firstRecs() throws Exception {
        ListMultipartUploadsRequest request = new ListMultipartUploadsRequest(
                bucketName ).withKeyMarker( keys[ 0 ] );
        MultipartUploadListing result = s3Client
                .listMultipartUploads( request );
        expUploads.clear();
        expUploads.add( keys[ 1 ], uploadIds.get( 1 ) );
        expUploads.add( keys[ 2 ], uploadIds.get( 2 ) );
        expUploads.add( keys[ 3 ], uploadIds.get( 3 ) );
        PartUploadUtils
                .checkListMultipartUploadsResults( result, expCommonPrefixes,
                        expUploads );
        runSuccessNum++;
    }

    @Test
    private void test_lastRecs() throws Exception {
        ListMultipartUploadsRequest request = new ListMultipartUploadsRequest(
                bucketName ).withKeyMarker( keys[ 3 ] );
        MultipartUploadListing result = s3Client
                .listMultipartUploads( request );
        expUploads.clear();
        PartUploadUtils
                .checkListMultipartUploadsResults( result, expCommonPrefixes,
                        expUploads );
        runSuccessNum++;
    }

    @Test
    private void test_ReturnLastRecs() throws Exception {
        ListMultipartUploadsRequest request = new ListMultipartUploadsRequest(
                bucketName ).withKeyMarker( keys[ 2 ] );
        MultipartUploadListing result = s3Client
                .listMultipartUploads( request );
        expUploads.clear();
        expUploads.add( keys[ 3 ], uploadIds.get( 3 ) );
        PartUploadUtils
                .checkListMultipartUploadsResults( result, expCommonPrefixes,
                        expUploads );
        runSuccessNum++;
    }

    @Test
    private void test_notExistRecs() throws Exception {
        ListMultipartUploadsRequest request = new ListMultipartUploadsRequest(
                bucketName ).withKeyMarker( "test5" );
        MultipartUploadListing result = s3Client
                .listMultipartUploads( request );
        expUploads.clear();
        PartUploadUtils
                .checkListMultipartUploadsResults( result, expCommonPrefixes,
                        expUploads );
        runSuccessNum++;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccessNum == expRunSuccessNum ) {
                s3Client.deleteBucket( bucketName );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            s3Client.shutdown();
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
}