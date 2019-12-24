package com.sequoias3.partupload;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.ListMultipartUploadsRequest;
import com.amazonaws.services.s3.model.MultipartUpload;
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
 * @Description seqDB-18756: lists in-progress multipart uploads by
 *              bucket.specify delimiter /prefix/keyMarker,.list query multiple.
 * @author wuyan
 * @Date 2019.08.06
 * @version 1.00
 */
public class ListMultipartUploadsWithCondition18756 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket18756";
    private String baseKeyName = "object18756.png";
    private int objectNum = 2015;
    private String prefix = "dir";
    private String delimiter = "/";
    private AmazonS3 s3Client = null;
    private MultiValueMap<String, String> uploads = new LinkedMultiValueMap<String, String>();

    @BeforeClass
    private void setUp() {
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );
    }

    @Test
    public void uploadParts() {
        List<String> commonPrefixes = initPartUpload();

        // specify keyMarker and uploadIdMarker
        int keySerial = 10;
        Object[] keyMarkers = uploads.keySet().toArray();
        Arrays.sort( keyMarkers );
        String keyMarker = keyMarkers[ keySerial ].toString();
        String uploadIdMarker = uploads.get( keyMarker ).get( 0 );
        List<String> expCommonPrefixes = commonPrefixes
                .subList( keySerial + 1, commonPrefixes.size() );

        // list multipartUploads and check list info.
        ListMultipartUploadsRequest request = new ListMultipartUploadsRequest(
                bucketName ).withDelimiter( delimiter ).withPrefix( prefix )
                .withKeyMarker( keyMarker )
                .withUploadIdMarker( uploadIdMarker );
        MultipartUploadListing result = null;
        List<String> matchPrefixList = new ArrayList<>();
        MultiValueMap<String, String> actUploads = new LinkedMultiValueMap<String, String>();
        int defaultEachListNum = 1000;
        do {
            result = s3Client.listMultipartUploads( request );
            List<String> eachCommonPrefixes = result.getCommonPrefixes();
            matchPrefixList.addAll( eachCommonPrefixes );
            List<MultipartUpload> multipartUploads = result
                    .getMultipartUploads();
            for ( MultipartUpload multipartUpload : multipartUploads ) {
                String keyName = multipartUpload.getKey();
                String uploadId = multipartUpload.getUploadId();
                actUploads.add( keyName, uploadId );
            }
            String continuationKeyMarker = result.getNextKeyMarker();
            String continuationUploadIdMarker = result.getUploadIdMarker();
            request.setKeyMarker( continuationKeyMarker );
            request.setUploadIdMarker( continuationUploadIdMarker );

            int eachListNums = eachCommonPrefixes.size();
            if ( eachListNums != defaultEachListNum && eachListNums
                    != expCommonPrefixes.size() % defaultEachListNum ) {
                Assert.fail( "list nums error! eachListNums: " + eachListNums );
            }

        } while ( result.isTruncated() );

        // check list result, the upload show num is 0
        Assert.assertEquals( actUploads.size(), 0,
                "the upload show num is 0!" );
        Assert.assertEquals( matchPrefixList, expCommonPrefixes,
                "actCommonPrefixes = " + matchPrefixList.toString()
                        + ",expCommonPrefixes = " + expCommonPrefixes
                        .toString() );
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
            String subKeyName = prefix + "test" + i + delimiter + baseKeyName;
            String uploadId1 = PartUploadUtils
                    .initPartUpload( s3Client, bucketName, subKeyName );

            String uploadId2 = PartUploadUtils
                    .initPartUpload( s3Client, bucketName, subKeyName );
            expCommonPrefixes.add( prefix + "test" + i + delimiter );
            uploads.add( subKeyName, uploadId1 );
            uploads.add( subKeyName, uploadId2 );
        }
        Collections.sort( expCommonPrefixes );
        return expCommonPrefixes;
    }

}
