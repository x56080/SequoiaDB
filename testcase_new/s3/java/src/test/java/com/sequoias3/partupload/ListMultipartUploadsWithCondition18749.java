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
 * @Description seqDB-18749: lists in-progress multipart uploads by
 *              bucket.specify delimiter and prefix
 * @author wuyan
 * @Date 2019.08.06
 * @version 1.00
 */
public class ListMultipartUploadsWithCondition18749 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket18749";
    private String baseKeyName = "object18749.png";
    private int objectNum = 20;
    private String prefix = "dir";
    private String delimiter = "/";
    private AmazonS3 s3Client = null;

    @BeforeClass
    private void setUp() {
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );
    }

    @Test
    public void uploadParts() {
        List<String> expCommonPrefixes = initPartUpload();
        // list multipartUploads and check list info.
        ListMultipartUploadsRequest request = new ListMultipartUploadsRequest(
                bucketName ).withDelimiter( delimiter ).withPrefix( prefix );
        MultipartUploadListing result = s3Client
                .listMultipartUploads( request );
        MultiValueMap<String, String> expUpload = new LinkedMultiValueMap<String, String>();
        PartUploadUtils
                .checkListMultipartUploadsResults( result, expCommonPrefixes,
                        expUpload );
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

    private List<String> initPartUpload() {
        List<String> expCommonPrefixes = new ArrayList<>();
        for ( int i = 0; i < objectNum; i++ ) {
            if ( i % 10 == 0 ) {
                // keyName misMatch prefix and delimiter
                String subKeyName = "test" + i + "_" + baseKeyName;
                PartUploadUtils
                        .initPartUpload( s3Client, bucketName, subKeyName );
                PartUploadUtils
                        .initPartUpload( s3Client, bucketName, subKeyName );
            } else {
                // keyName match prefix and delimter
                String subKeyName = prefix + i + delimiter + "_" + baseKeyName;
                PartUploadUtils
                        .initPartUpload( s3Client, bucketName, subKeyName );
                PartUploadUtils
                        .initPartUpload( s3Client, bucketName, subKeyName );
                expCommonPrefixes.add( prefix + i + delimiter );
            }
        }
        return expCommonPrefixes;
    }

}
