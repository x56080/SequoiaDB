package com.sequoias3.partupload;

import com.amazonaws.services.s3.AmazonS3;
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

import java.util.ArrayList;
import java.util.List;

/**
 * @Description seqDB-18743: lists in-progress multipart uploads by bucket.specify prefix:matching
 *              prefix and mismatched prefix.
 * @author wuyan
 * @Date 2019.08.05
 * @version 1.00
 */
public class ListMultipartUploadsWithDelimiter18743 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket18743";
    private String[] keyNames = { "atest0_18743.png", "dir1/a/test1_18743.png",
            "dir1/dir2/a/test2_18743.png", "dirtest1/test3_18743.png",
            "dir4/test4_18743.png" };
    private AmazonS3 s3Client = null;

    @BeforeClass
    private void setUp() {
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
    }

    @Test
    public void uploadParts() {
        MultiValueMap<String, String> uploadIds = new LinkedMultiValueMap<String, String>();
        for ( String keyName : keyNames ) {
            String uploadId = PartUploadUtils
                    .initPartUpload( s3Client, bucketName, keyName );
            uploadIds.add( keyName, uploadId );
        }
        // test a: matching prefix
        String prefixA = "dir";
        listPartUploadsMatchedPrefix( uploadIds, prefixA );
        // test b: mis matched prefix
        String prefixB = "/dir";
        listPartUploadsMisMatchedPrefix( uploadIds, prefixB );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                CommLib.clearBucket( s3Client, bucketName );
            }
        } finally {
            s3Client.shutdown();
        }
    }

    private void listPartUploadsMatchedPrefix(
            MultiValueMap<String, String> uploadIds, String prefix ) {
        ListMultipartUploadsRequest request = new ListMultipartUploadsRequest(
                bucketName ).withPrefix( prefix );
        MultipartUploadListing result = s3Client
                .listMultipartUploads( request );
        MultiValueMap<String, String> expUpload = uploadIds;
        // remove the object not match prefix
        expUpload.remove( keyNames[ 0 ] );

        List<String> expCommonPrefixes = new ArrayList<>();
        PartUploadUtils
                .checkListMultipartUploadsResults( result, expCommonPrefixes,
                        expUpload );
    }

    private void listPartUploadsMisMatchedPrefix(
            MultiValueMap<String, String> uploadIds, String prefix ) {
        ListMultipartUploadsRequest request = new ListMultipartUploadsRequest(
                bucketName ).withPrefix( prefix );
        MultipartUploadListing result = s3Client
                .listMultipartUploads( request );

        List<String> expCommonPrefixes = new ArrayList<>();
        MultiValueMap<String, String> expUploadIds = new LinkedMultiValueMap<String, String>();
        PartUploadUtils
                .checkListMultipartUploadsResults( result, expCommonPrefixes,
                        expUploadIds );
    }

}
