package com.sequoias3.partupload;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.BucketVersioningConfiguration;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.ListMultipartUploadsRequest;
import com.amazonaws.services.s3.model.MultipartUploadListing;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.PartUploadUtils;
import org.springframework.util.LinkedMultiValueMap;
import org.springframework.util.MultiValueMap;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

/**
 * @Description seqDB-18753:带prefix、keyMarker、uploadIdMarker和delimiter查询桶分段上传列表，
 *              不匹配其中一个条件
 * @Author wangkexin
 * @Date 2019.08.06
 */

public class ListMultipartUploads18753 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket18753";
    private String[] keyNames = { "dir1/a18753", "dir1/dir2/test18753",
            "dir1a/test18753", "dir1b/18753", "dir1c_test18753", "test18753" };
    private AmazonS3 s3Client = null;

    @BeforeClass
    private void setUp() throws IOException {
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );
        CommLib.setBucketVersioning( s3Client, bucketName,
                BucketVersioningConfiguration.ENABLED );
    }

    @Test
    private void testListMultipartUploads() throws Exception {
        List<String> uploadIds1 = new ArrayList<>();
        List<String> uploadIds2 = new ArrayList<>();
        String uploadId = "";
        for ( String keyName : keyNames ) {
            uploadId = PartUploadUtils
                    .initPartUpload( s3Client, bucketName, keyName );
            uploadIds1.add( uploadId );
            uploadId = PartUploadUtils
                    .initPartUpload( s3Client, bucketName, keyName );
            uploadIds2.add( uploadId );
        }

        // 不匹配delimiter
        ListMultipartUploadsRequest request = new ListMultipartUploadsRequest(
                bucketName );
        request.setPrefix( "dir" );
        // keyMarKer:"dir1a/test18753"
        request.setKeyMarker( keyNames[ 2 ] );
        request.setUploadIdMarker( uploadIds1.get( 2 ) );
        request.setDelimiter( "%" );
        MultipartUploadListing partUploadList = s3Client
                .listMultipartUploads( request );
        List<String> expCommonPrefixes = new ArrayList<>();
        MultiValueMap<String, String> expUploads = new LinkedMultiValueMap<String, String>();
        expUploads.add( keyNames[ 2 ], uploadIds2.get( 2 ) );
        expUploads.add( keyNames[ 3 ], uploadIds1.get( 3 ) );
        expUploads.add( keyNames[ 3 ], uploadIds2.get( 3 ) );
        expUploads.add( keyNames[ 4 ], uploadIds1.get( 4 ) );
        expUploads.add( keyNames[ 4 ], uploadIds2.get( 4 ) );
        PartUploadUtils.checkListMultipartUploadsResults( partUploadList,
                expCommonPrefixes, expUploads );

        // 不匹配prefix
        ListMultipartUploadsRequest request2 = new ListMultipartUploadsRequest(
                bucketName );
        request2.setPrefix( "prefix" );
        request2.setKeyMarker( keyNames[ 2 ] );
        request2.setUploadIdMarker( uploadIds1.get( 2 ) );
        request2.setDelimiter( "/" );
        partUploadList = s3Client.listMultipartUploads( request2 );
        expUploads = new LinkedMultiValueMap<String, String>();
        PartUploadUtils.checkListMultipartUploadsResults( partUploadList,
                expCommonPrefixes, expUploads );

        // 不匹配uploadIdMarker
        ListMultipartUploadsRequest request3 = new ListMultipartUploadsRequest(
                bucketName );
        request3.setPrefix( "prefix" );
        request3.setKeyMarker( keyNames[ 2 ] );
        request3.setUploadIdMarker( "123456" );
        request3.setDelimiter( "/" );
        partUploadList = s3Client.listMultipartUploads( request3 );
        PartUploadUtils.checkListMultipartUploadsResults( partUploadList,
                expCommonPrefixes, expUploads );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                CommLib.clearBucket( s3Client, bucketName );
            }
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }
}
