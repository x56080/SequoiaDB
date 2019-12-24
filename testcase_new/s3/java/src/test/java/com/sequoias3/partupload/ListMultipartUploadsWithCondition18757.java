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
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

/**
 * @Description seqDB-18757: lists in-progress multipart uploads by
 *              bucket.specify delimiter/prefix/keyMarker/uploadIdMarker,list
 *              query again with different condition.
 * @author wuyan
 * @Date 2019.08.06
 * @version 1.00
 */
public class ListMultipartUploadsWithCondition18757 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket18757";
    private String baseKeyName = "object18757.png";
    private int objectNum = 1050;
    private int defaultEachListMaxNum = 1000;
    private String delimiter1 = "/";
    private String delimiter2 = "test";
    private AmazonS3 s3Client = null;
    private List<String> commonPrefixes1 = new ArrayList<>();
    private List<String> commonPrefixes2 = new ArrayList<>();
    private MultiValueMap<String, String> uploads = new LinkedMultiValueMap<String, String>();
    private MultiValueMap<String, String> uploads2 = new LinkedMultiValueMap<String, String>();

    @BeforeClass
    private void setUp() {
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );
        initPartUpload();
    }

    @Test
    public void uploadParts() {
        // first query,specify keyMarker and uploadIdMarker
        int keySerial = 5;
        Object[] keyMarkers = uploads.keySet().toArray();
        Arrays.sort( keyMarkers );
        String keyMarker = keyMarkers[ keySerial ].toString();
        String uploadIdMarker = uploads.get( keyMarker ).get( 0 );
        String prefix1 = "dir";

        // list multipartUploads and check list info.return the num is 1000
        ListMultipartUploadsRequest request = new ListMultipartUploadsRequest(
                bucketName ).withDelimiter( delimiter1 ).withPrefix( prefix1 )
                .withKeyMarker( keyMarker )
                .withUploadIdMarker( uploadIdMarker );
        MultipartUploadListing result = s3Client
                .listMultipartUploads( request );
        MultiValueMap<String, String> uploads1 = new LinkedMultiValueMap<String, String>();
        List<String> expCommonPrefixes1 = commonPrefixes1
                .subList( keySerial + 1,
                        defaultEachListMaxNum + keySerial + 1 );
        PartUploadUtils
                .checkListMultipartUploadsResults( result, expCommonPrefixes1,
                        uploads1 );

        // second query,reSet delimiter and prefix.
        String continuationKeyMarker = result.getNextKeyMarker();
        String continuationUploadIdMarker = result.getNextUploadIdMarker();
        String prefix2 = "dir_";
        request = new ListMultipartUploadsRequest( bucketName )
                .withDelimiter( delimiter2 ).withPrefix( prefix2 )
                .withKeyMarker( continuationKeyMarker )
                .withUploadIdMarker( continuationUploadIdMarker );
        MultipartUploadListing result2 = s3Client
                .listMultipartUploads( request );
        List<String> expCommonPrefixes2 = commonPrefixes2
                .subList( defaultEachListMaxNum + keySerial + 1,
                        commonPrefixes2.size() );
        PartUploadUtils
                .checkListMultipartUploadsResults( result2, expCommonPrefixes2,
                        uploads2 );
        Assert.assertFalse( result2.isTruncated(),
                "the list should be query	finsh!" );
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

    private void initPartUpload() {
        for ( int i = 0; i < objectNum - 1; i++ ) {
            String subKeyName =
                    "dir_" + i + delimiter2 + delimiter1 + baseKeyName;
            String uploadId1 = PartUploadUtils
                    .initPartUpload( s3Client, bucketName, subKeyName );
            String uploadId2 = PartUploadUtils
                    .initPartUpload( s3Client, bucketName, subKeyName );
            commonPrefixes1.add( "dir_" + i + delimiter2 + delimiter1 );
            commonPrefixes2.add( "dir_" + i + delimiter2 );
            uploads.add( subKeyName, uploadId1 );
            uploads.add( subKeyName, uploadId2 );
        }
        // add the object is misMatch delimiter1 and delimieter2
        String subKeyName = "dir_998_" + baseKeyName;
        String uploadId1 = PartUploadUtils
                .initPartUpload( s3Client, bucketName, subKeyName );
        String uploadId2 = PartUploadUtils
                .initPartUpload( s3Client, bucketName, subKeyName );
        uploads2.add( subKeyName, uploadId1 );
        uploads2.add( subKeyName, uploadId2 );
        Collections.sort( commonPrefixes1 );
        Collections.sort( commonPrefixes2 );
    }

}
