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
 * @Description seqDB-18760: lists in-progress multipart uploads by
 *              bucket.specify delimiter/prefix/keyMarker/uploadIdMarker/maxkeys
 * @author wuyan
 * @Date 2019.08.06
 * @version 1.00
 */
public class ListMultipartUploadsWithCondition18760 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket18760";
    private String keyNameA = "/aa!_test1_18760";
    private String keyNameB = "/aa/bb/test2_18760";
    private String keyNameC = "/aa/bb/test3_18760";
    private String keyNameD = "/aa?aa?test4_18760";
    private String keyNameE = "/aatest%_test5_18760";
    private String keyNameF = "/aaftest/_test6_18760";
    private AmazonS3 s3Client = null;

    @BeforeClass
    private void setUp() {
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
    }

    @Test
    public void listMultipartUploads() {
        String uploadIdA = PartUploadUtils.initPartUpload( s3Client, bucketName,
                keyNameA );
        PartUploadUtils.initPartUpload( s3Client, bucketName, keyNameB );
        PartUploadUtils.initPartUpload( s3Client, bucketName, keyNameC );
        String uploadIdD1 = PartUploadUtils.initPartUpload( s3Client,
                bucketName, keyNameD );
        String uploadIdD2 = PartUploadUtils.initPartUpload( s3Client,
                bucketName, keyNameD );
        String uploadIdE = PartUploadUtils.initPartUpload( s3Client, bucketName,
                keyNameE );
        PartUploadUtils.initPartUpload( s3Client, bucketName, keyNameF );

        int maxUploads = 2;
        String prefix = "/aa";
        String delimiter = "/";
        // first query
        ListMultipartUploadsRequest request = new ListMultipartUploadsRequest(
                bucketName ).withMaxUploads( maxUploads )
                        .withDelimiter( delimiter ).withPrefix( prefix );
        MultipartUploadListing result1 = s3Client
                .listMultipartUploads( request );
        MultiValueMap< String, String > expUpload1 = new LinkedMultiValueMap< String, String >();
        expUpload1.add( keyNameA, uploadIdA );
        List< String > expCommonPrefixes1 = new ArrayList<>();
        expCommonPrefixes1.add( "/aa/" );
        PartUploadUtils.checkListMultipartUploadsResults( result1,
                expCommonPrefixes1, expUpload1 );

        // second query, the nextKeyMarKey is match the "/aa/" in commprefix
        String nextKeyMarker1 = result1.getNextKeyMarker();
        String nextUploadId1 = result1.getNextUploadIdMarker();
        Assert.assertEquals( nextKeyMarker1, "/aa/" );
        Assert.assertEquals( nextUploadId1, null );
        request = new ListMultipartUploadsRequest( bucketName )
                .withMaxUploads( maxUploads ).withKeyMarker( nextKeyMarker1 )
                .withUploadIdMarker( nextUploadId1 ).withDelimiter( delimiter )
                .withPrefix( prefix );
        MultipartUploadListing result2 = s3Client
                .listMultipartUploads( request );
        MultiValueMap< String, String > expUpload2 = new LinkedMultiValueMap< String, String >();
        expUpload2.add( keyNameD, uploadIdD1 );
        expUpload2.add( keyNameD, uploadIdD2 );
        List< String > expCommonPrefixes2 = new ArrayList<>();
        PartUploadUtils.checkListMultipartUploadsResults( result2,
                expCommonPrefixes2, expUpload2 );

        // third query, the nextKeyMarKey is match the keyNameD and uploadIdD2
        String nextKeyMarker2 = result2.getNextKeyMarker();
        String nextUploadId2 = result2.getNextUploadIdMarker();
        Assert.assertEquals( nextKeyMarker2, keyNameD );
        Assert.assertEquals( nextUploadId2, uploadIdD2 );

        request = new ListMultipartUploadsRequest( bucketName )
                .withMaxUploads( maxUploads ).withKeyMarker( nextKeyMarker2 )
                .withUploadIdMarker( nextUploadId2 ).withDelimiter( delimiter )
                .withPrefix( prefix );
        MultipartUploadListing result3 = s3Client
                .listMultipartUploads( request );
        MultiValueMap< String, String > expUpload3 = new LinkedMultiValueMap< String, String >();
        expUpload3.add( keyNameE, uploadIdE );
        List< String > expCommonPrefixes3 = new ArrayList<>();
        expCommonPrefixes3.add( "/aaftest/" );
        PartUploadUtils.checkListMultipartUploadsResults( result3,
                expCommonPrefixes3, expUpload3 );
        Assert.assertFalse( result3.isTruncated(),
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
