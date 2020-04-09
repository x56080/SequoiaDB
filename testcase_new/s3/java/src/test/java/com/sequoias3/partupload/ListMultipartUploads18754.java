package com.sequoias3.partupload;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.BucketVersioningConfiguration;
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
 * @Description seqDB-18754:带delimiter和maxkeys查询桶分段上传列表
 * @Author wangkexin
 * @Date 2019.08.06
 */

public class ListMultipartUploads18754 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket18754";
    private String[] keyNames = { "dir1/a18754", "dir1/dir2/test18754",
            "dir1a/test18754", "dir1b/18754", "dir1c_test18754", "test18754" };
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
        String uploadId = "";
        for ( String keyName : keyNames ) {
            uploadId = PartUploadUtils.initPartUpload( s3Client, bucketName,
                    keyName );
            uploadIds1.add( uploadId );
            uploadId = PartUploadUtils.initPartUpload( s3Client, bucketName,
                    keyName );
            uploadIds2.add( uploadId );
        }

        // 指定maxkeys一次返回所有匹配条件的对象，指定为3
        int maxKeys = 3;
        List< String > expCommonPrefixes = new ArrayList<>();
        expCommonPrefixes.add( "dir1/" );
        expCommonPrefixes.add( "dir1a/" );
        expCommonPrefixes.add( "dir1b/" );
        MultiValueMap< String, String > expUploads = new LinkedMultiValueMap< String, String >();
        expUploads.add( keyNames[ 4 ], uploadIds1.get( 4 ) );
        expUploads.add( keyNames[ 4 ], uploadIds2.get( 4 ) );
        expUploads.add( keyNames[ 5 ], uploadIds1.get( 5 ) );
        expUploads.add( keyNames[ 5 ], uploadIds2.get( 5 ) );
        checkListMultipartUploadsWithMaxKeys( maxKeys, expCommonPrefixes,
                expUploads );

        // 指定maxkeys多次返回所有匹配条件的对象，指定为1
        maxKeys = 1;
        checkListMultipartUploadsWithMaxKeys( maxKeys, expCommonPrefixes,
                expUploads );
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

    private void checkListMultipartUploadsWithMaxKeys( int maxKeys,
            List< String > expCommonPrefixes,
            MultiValueMap< String, String > expUploads ) {
        ListMultipartUploadsRequest request = new ListMultipartUploadsRequest(
                bucketName );
        request.setDelimiter( "/" );
        request.setMaxUploads( maxKeys );
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
            if ( returnedUploadNum > maxKeys ) {
                Assert.fail( "returnedUploadNum = " + returnedUploadNum
                        + ", maxKeys = " + maxKeys + ", commonprefixes : "
                        + actCommonPrefixes.toString() + " uploads:"
                        + actUploads.toString() );
            }

        } while ( partUploadList.isTruncated() );

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
