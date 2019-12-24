package com.sequoias3.partupload;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AbortMultipartUploadRequest;
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
 * @Description seqDB-18758: lists in-progress multipart uploads by
 *              bucket.specify that the nextContinuationToken match object and
 *              uploadId is deleted.
 * @author wuyan
 * @Date 2019.08.06
 * @version 1.00
 */
public class ListMultipartUploads18758 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket18758";
    private String keyNameA = "a/test0_18758";
    private String keyNameB = "a/test1_18758";
    private String keyNameC = "dir1/atest2_18758.png";
    private AmazonS3 s3Client = null;

    @BeforeClass
    private void setUp() {
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
    }

    @Test
    public void listMultipartUploads() {
        String uploadIdA1 = PartUploadUtils
                .initPartUpload( s3Client, bucketName, keyNameA );
        String uploadIdA2 = PartUploadUtils
                .initPartUpload( s3Client, bucketName, keyNameA );
        String uploadIdB1 = PartUploadUtils
                .initPartUpload( s3Client, bucketName, keyNameB );
        String uploadIdB2 = PartUploadUtils
                .initPartUpload( s3Client, bucketName, keyNameB );
        String uploadIdC1 = PartUploadUtils
                .initPartUpload( s3Client, bucketName, keyNameC );
        String uploadIdC2 = PartUploadUtils
                .initPartUpload( s3Client, bucketName, keyNameC );
        int maxUploads = 3;
        // first query
        ListMultipartUploadsRequest request = new ListMultipartUploadsRequest(
                bucketName ).withMaxUploads( maxUploads );
        MultipartUploadListing result1 = s3Client
                .listMultipartUploads( request );
        MultiValueMap<String, String> expUpload1 = new LinkedMultiValueMap<String, String>();
        expUpload1.add( keyNameA, uploadIdA1 );
        expUpload1.add( keyNameA, uploadIdA2 );
        expUpload1.add( keyNameB, uploadIdB1 );
        List<String> expCommonPrefixes = new ArrayList<>();
        PartUploadUtils
                .checkListMultipartUploadsResults( result1, expCommonPrefixes,
                        expUpload1 );

        // abortMultipartUpload by the nextKeyMarker(eg:keyNameB:uploadIdB1)
        String nextKeyMarker = result1.getNextKeyMarker();
        String nextUploadId = result1.getNextUploadIdMarker();
        AbortMultipartUploadRequest abortRequest = new AbortMultipartUploadRequest(
                bucketName, nextKeyMarker, nextUploadId );
        s3Client.abortMultipartUpload( abortRequest );

        // second query
        request = new ListMultipartUploadsRequest( bucketName )
                .withKeyMarker( nextKeyMarker )
                .withUploadIdMarker( nextUploadId );
        MultipartUploadListing result2 = s3Client
                .listMultipartUploads( request );
        MultiValueMap<String, String> expUpload2 = new LinkedMultiValueMap<String, String>();
        expUpload2.add( keyNameB, uploadIdB2 );
        expUpload2.add( keyNameC, uploadIdC1 );
        expUpload2.add( keyNameC, uploadIdC2 );
        PartUploadUtils
                .checkListMultipartUploadsResults( result2, expCommonPrefixes,
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
