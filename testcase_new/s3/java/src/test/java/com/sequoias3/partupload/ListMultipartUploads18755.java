package com.sequoias3.partupload;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CreateBucketRequest;
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

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;

/**
 * @Description seqDB-18755:带delimiter和maxkeys查询桶分段上传列表，只匹配其中一个条件
 * @Author wangkexin
 * @Date 2019.08.06
 */

public class ListMultipartUploads18755 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket18755";
    private String[] keyNames = { "dir1/a18755", "dir1/dir2/test18755",
            "dir1a/test18755", "dir1b/18755", "dir1c_test18755", "test18755" };
    private AmazonS3 s3Client = null;

    @BeforeClass
    private void setUp() throws IOException {
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );
    }

    @Test
    private void testListMultipartUploads() throws Exception {
        MultiValueMap< String, String > expUploads = new LinkedMultiValueMap< String, String >();
        List< String > uploadIds = new ArrayList<>();
        for ( String keyName : keyNames ) {
            String uploadId = PartUploadUtils.initPartUpload( s3Client,
                    bucketName, keyName );
            uploadIds.add( uploadId );
            expUploads.add( keyName, uploadId );
        }

        // 不匹配delimiter条件
        List< String > expCommonPrefixes = new ArrayList<>();
        listAndCheckResult( "%", 3, 3, expCommonPrefixes, expUploads );

        // 不匹配maxUploads条件
        expCommonPrefixes.add( "dir1/" );
        expCommonPrefixes.add( "dir1a/" );
        expCommonPrefixes.add( "dir1b/" );
        MultiValueMap< String, String > temPxpUploads = new LinkedMultiValueMap< String, String >();
        temPxpUploads.put( keyNames[ 4 ], expUploads.get( keyNames[ 4 ] ) );
        temPxpUploads.put( keyNames[ 5 ], expUploads.get( keyNames[ 5 ] ) );
        listAndCheckResult( "/", 10,
                expCommonPrefixes.size() + temPxpUploads.size(),
                expCommonPrefixes, temPxpUploads );
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

    private void listAndCheckResult( String delimiter, int maxUploads,
            int expReturnedUploadNum, List< String > expCommonPrefixes,
            MultiValueMap< String, String > expUploads ) {
        ListMultipartUploadsRequest request = new ListMultipartUploadsRequest(
                bucketName );
        request.setDelimiter( delimiter );
        request.setMaxUploads( maxUploads );
        MultipartUploadListing partUploadList;
        List< String > actCommonPrefixes = new ArrayList<>();
        MultiValueMap< String, String > actUploads = new LinkedMultiValueMap< String, String >();
        do {
            int returnedUploadNum = 0;
            partUploadList = s3Client.listMultipartUploads( request );
            List< String > commonPrefixes = partUploadList.getCommonPrefixes();
            returnedUploadNum += commonPrefixes.size();
            actCommonPrefixes.addAll( commonPrefixes );
            List< MultipartUpload > multipartUploads = partUploadList
                    .getMultipartUploads();
            for ( MultipartUpload multipartUpload : multipartUploads ) {
                String temKeyName = multipartUpload.getKey();
                String temUploadId = multipartUpload.getUploadId();
                actUploads.add( temKeyName, temUploadId );
            }
            returnedUploadNum += multipartUploads.size();

            String nextKeyMarKer = partUploadList.getNextKeyMarker();
            request.setKeyMarker( nextKeyMarKer );
            String nextUploadIdMarker = partUploadList.getNextUploadIdMarker();
            request.setUploadIdMarker( nextUploadIdMarker );
            Assert.assertEquals( returnedUploadNum, expReturnedUploadNum,
                    "commonprefixes : " + actCommonPrefixes.toString()
                            + " uploads:" + actUploads.toString() );
        } while ( partUploadList.isTruncated() );
        checkResult( expCommonPrefixes, actCommonPrefixes, expUploads,
                actUploads );
    }

    private void checkResult( List< String > expCommonPrefixes,
            List< String > actCommonPrefixes,
            MultiValueMap< String, String > expUploads,
            MultiValueMap< String, String > actUploads ) {
        Assert.assertEquals( actCommonPrefixes, expCommonPrefixes,
                "actCommonPrefixes = " + actCommonPrefixes.toString()
                        + ",expCommonPrefixes = "
                        + expCommonPrefixes.toString() );
        Assert.assertEquals( actUploads.size(), expUploads.size(),
                "actMap = " + actUploads.toString() + ",expUpload = "
                        + expUploads.toString() );
        for ( Map.Entry< String, List< String > > entry : expUploads
                .entrySet() ) {
            Assert.assertEquals( actUploads.get( entry.getKey() ),
                    expUploads.get( entry.getKey() ),
                    "actMap = " + actUploads.toString() + ",expMap = "
                            + expUploads.toString() );
        }
    }
}
