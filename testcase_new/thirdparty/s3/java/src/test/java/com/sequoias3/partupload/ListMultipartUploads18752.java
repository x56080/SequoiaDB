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
 * @Description seqDB-18752:带prefix、keyMarker、
 *              uploadIdMarker和delimiter匹配查询桶分段上传列表
 * @Author wangkexin
 * @Date 2019.08.05
 */

public class ListMultipartUploads18752 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket18752";
    private String[] keyNames = { "dir1/a18752", "dir1/dir2/test18752",
            "dir1a/test18752", "dir1b/18752", "test18752" };
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
        List< String > uploadIds1 = new ArrayList<>();
        List< String > uploadIds2 = new ArrayList<>();
        List< String > uploadIds3 = new ArrayList<>();
        String uploadId = "";
        for ( String keyName : keyNames ) {
            uploadId = PartUploadUtils.initPartUpload( s3Client, bucketName,
                    keyName );
            uploadIds1.add( uploadId );
            uploadId = PartUploadUtils.initPartUpload( s3Client, bucketName,
                    keyName );
            uploadIds2.add( uploadId );
            uploadId = PartUploadUtils.initPartUpload( s3Client, bucketName,
                    keyName );
            uploadIds3.add( uploadId );
        }
        ListMultipartUploadsRequest request = new ListMultipartUploadsRequest(
                bucketName );
        request.setPrefix( "dir" );
        // keyMarKer:"dir1/dir2/test18752"
        request.setKeyMarker( keyNames[ 1 ] );
        request.setUploadIdMarker( uploadIds2.get( 1 ) );
        request.setDelimiter( "/" );
        MultipartUploadListing partUploadList = s3Client
                .listMultipartUploads( request );
        List< String > expCommonPrefixes = new ArrayList<>();
        expCommonPrefixes.add( "dir1a/" );
        expCommonPrefixes.add( "dir1b/" );
        MultiValueMap< String, String > expUploads = new LinkedMultiValueMap< String, String >();
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
