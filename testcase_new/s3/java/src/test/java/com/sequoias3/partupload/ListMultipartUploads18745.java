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
 * @Description seqDB-18745:带uploadIdMarker查询桶分段上传列表
 * @Author huangxiaoni
 * @Date 2019.07.26
 */

public class ListMultipartUploads18745 extends S3TestBase {
    private boolean runSuccess = false;
    private AmazonS3 s3Client;
    private String bucketName = "bucket18745";
    private File localPath;
    private String filePath;
    private File file;
    private long fileSize = 11 * 1024 * 1024;
    private int maxPartNumber = 2;
    private String[] keys = { "/aa/bb/test18745_1", "/aa/bb/test18745_2",
            "test18745_3", "test18745_4" };
    private List<String> uploadIds = new ArrayList<>();

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
    private void test() throws Exception {
        ListMultipartUploadsRequest request = new ListMultipartUploadsRequest(
                bucketName ).withUploadIdMarker( uploadIds.get( 1 ) );
        MultipartUploadListing result = s3Client
                .listMultipartUploads( request );

        List<String> expCommonPrefixes = new ArrayList<>();
        MultiValueMap<String, String> expUploads = new LinkedMultiValueMap<String, String>();
        expUploads.add( keys[ 0 ], uploadIds.get( 0 ) );
        expUploads.add( keys[ 1 ], uploadIds.get( 1 ) );
        expUploads.add( keys[ 2 ], uploadIds.get( 2 ) );
        expUploads.add( keys[ 3 ], uploadIds.get( 3 ) );
        PartUploadUtils
                .checkListMultipartUploadsResults( result, expCommonPrefixes,
                        expUploads );

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