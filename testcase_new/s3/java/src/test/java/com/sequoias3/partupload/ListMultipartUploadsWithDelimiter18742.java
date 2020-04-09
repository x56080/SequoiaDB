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
 * @Description seqDB-18742: lists in-progress multipart uploads by
 *              bucket.specify delimiter:matching delimiter and mismatched
 *              delimiter.
 * @author wuyan
 * @Date 2019.08.05
 * @version 1.00
 */
public class ListMultipartUploadsWithDelimiter18742 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket18742";
    private String[] keyNames = { "atest0_18742.png", "dir1/a/test1_18742.png",
            "dir1/dir2/a/test2_18742.png", "/a/test3_18742.png",
            "dir4/test4_18742.png" };
    private AmazonS3 s3Client = null;

    @BeforeClass
    private void setUp() {
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
    }

    @Test
    public void uploadParts() {
        MultiValueMap< String, String > uploadIds = new LinkedMultiValueMap< String, String >();
        for ( String keyName : keyNames ) {
            String uploadId = PartUploadUtils.initPartUpload( s3Client,
                    bucketName, keyName );
            uploadIds.add( keyName, uploadId );
        }

        // test a: matching delimiter
        String delimiterA = "/";
        listPartUploadsMatchedDelimiter( uploadIds, delimiterA );
        // test b: mis matched delimiter
        String delimiterB = "/testb";
        listPartUploadsMisMatchedDelimiter( uploadIds, delimiterB );
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

    private void listPartUploadsMatchedDelimiter(
            MultiValueMap< String, String > uploadIds, String delimiter ) {
        ListMultipartUploadsRequest request = new ListMultipartUploadsRequest(
                bucketName ).withDelimiter( delimiter );
        MultipartUploadListing result = s3Client
                .listMultipartUploads( request );
        MultiValueMap< String, String > expUpload = new LinkedMultiValueMap< String, String >();
        expUpload.add( keyNames[ 0 ], uploadIds.getFirst( keyNames[ 0 ] ) );
        List< String > expCommonPrefixes = new ArrayList<>();
        expCommonPrefixes.add( "dir1/" );
        expCommonPrefixes.add( "/" );
        expCommonPrefixes.add( "dir4/" );
        PartUploadUtils.checkListMultipartUploadsResults( result,
                expCommonPrefixes, expUpload );
    }

    private void listPartUploadsMisMatchedDelimiter(
            MultiValueMap< String, String > uploadIds, String delimiter ) {
        ListMultipartUploadsRequest request = new ListMultipartUploadsRequest(
                bucketName ).withDelimiter( delimiter );
        MultipartUploadListing result = s3Client
                .listMultipartUploads( request );

        List< String > expCommonPrefixes = new ArrayList<>();
        PartUploadUtils.checkListMultipartUploadsResults( result,
                expCommonPrefixes, uploadIds );
    }

}
