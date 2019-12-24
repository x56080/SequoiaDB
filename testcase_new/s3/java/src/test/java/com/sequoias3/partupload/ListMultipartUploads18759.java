package com.sequoias3.partupload;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.ListMultipartUploadsRequest;
import com.amazonaws.services.s3.model.MultipartUploadListing;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.PartUploadUtils;
import org.springframework.util.LinkedMultiValueMap;
import org.springframework.util.MultiValueMap;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.ArrayList;
import java.util.List;

/**
 * @Description seqDB-18759: lists in-progress multipart uploads by
 *              bucket.Multiple query results match the same record in
 *              commprefix.
 * @author wuyan
 * @Date 2019.08.06
 * @version 1.00
 */
public class ListMultipartUploads18759 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket18759";
    private String keyNameA = "/aa!_test1_18759";
    private String keyNameB = "/aa/bb/test2_18759";
    private String keyNameC = "/aa/bb/test3_18759";
    private String keyNameD = "/aa?aa?test4_18759";
    private String keyNameE = "/aa/_test5_18759";
    private String keyNameF = "/aa_test6_18759";
    private AmazonS3 s3Client = null;

    @BeforeClass
    private void setUp() {
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
    }

    @Test
    public void listMultipartUploads() {
        String uploadIdA = PartUploadUtils
                .initPartUpload( s3Client, bucketName, keyNameA );
        PartUploadUtils.initPartUpload( s3Client, bucketName, keyNameB );
        PartUploadUtils.initPartUpload( s3Client, bucketName, keyNameC );
        String uploadIdD = PartUploadUtils
                .initPartUpload( s3Client, bucketName, keyNameD );
        int maxUploads = 2;
        String prefix = "/aa";
        String delimiter = "/";
        // first query
        ListMultipartUploadsRequest request = new ListMultipartUploadsRequest(
                bucketName ).withMaxUploads( maxUploads )
                .withDelimiter( delimiter ).withPrefix( prefix );
        MultipartUploadListing result1 = s3Client
                .listMultipartUploads( request );
        MultiValueMap<String, String> expUpload1 = new LinkedMultiValueMap<String, String>();
        expUpload1.add( keyNameA, uploadIdA );
        List<String> expCommonPrefixes1 = new ArrayList<>();
        expCommonPrefixes1.add( "/aa/" );
        PartUploadUtils
                .checkListMultipartUploadsResults( result1, expCommonPrefixes1,
                        expUpload1 );

        // second list, match the new putObject(
        // keyNameE:"/aa/_test5_18759",keyNameF = "/aa_test6_18759")
        PartUploadUtils.initPartUpload( s3Client, bucketName, keyNameE );
        String uploadIdF = PartUploadUtils
                .initPartUpload( s3Client, bucketName, keyNameF );
        String nextKeyMarker = result1.getNextKeyMarker();
        String nextUploadId = result1.getNextUploadIdMarker();

        // second query
        request = new ListMultipartUploadsRequest( bucketName )
                .withKeyMarker( nextKeyMarker )
                .withUploadIdMarker( nextUploadId ).withDelimiter( delimiter )
                .withPrefix( prefix );
        MultipartUploadListing result2 = s3Client
                .listMultipartUploads( request );
        MultiValueMap<String, String> expUpload2 = new LinkedMultiValueMap<String, String>();
        expUpload2.add( keyNameD, uploadIdD );
        expUpload2.add( keyNameF, uploadIdF );
        List<String> expCommonPrefixes2 = new ArrayList<>();
        PartUploadUtils
                .checkListMultipartUploadsResults( result2, expCommonPrefixes2,
                        expUpload2 );
        Assert.assertFalse( result2.isTruncated(),
                "the list query should be finsh!" );

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
}
