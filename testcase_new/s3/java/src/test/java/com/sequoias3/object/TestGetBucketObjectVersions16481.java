package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.ListVersionsRequest;
import com.amazonaws.services.s3.model.VersionListing;
import com.sequoiadb.exception.BaseException;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.UnsupportedEncodingException;
import java.net.URLEncoder;
import java.util.List;

/**
 * @Description: GetBucketObjectVersion接口参数校验 testlink-case: seqDB-16481
 *
 * @author wangkexin
 * @Date 2019.01.08
 * @version 1.00
 */
public class TestGetBucketObjectVersions16481 extends S3TestBase {
    private String bucketName = "bucket16481";
    private AmazonS3 s3Client = null;
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );
        s3Client.putObject( bucketName, "aa/bb/key16481_1/123",
                "content16481_1" );
        s3Client.putObject( bucketName, "aa/bb/key16481_2/456",
                "content16481_2" );
        s3Client.putObject( bucketName, "aa/bb/key16481_3/789",
                "content16481_3" );
        s3Client.putObject( bucketName, "aa/bb/key16481_4/000",
                "content16481_4" );
    }

    @Test
    public void testListObjectsV2() throws UnsupportedEncodingException {
        // test a : 合法值
        ListVersionsRequest request = new ListVersionsRequest();
        request.setBucketName( bucketName );
        request.setDelimiter( "/" );
        request.setEncodingType( "url" );
        request.setKeyMarker( "aa/bb/key16481_2/456" );
        request.setMaxResults( 1 );
        request.setPrefix( "aa/bb/" );
        request.setVersionIdMarker( "null" );
        VersionListing list = s3Client.listVersions( request );

        List< String > commonPrefixes = list.getCommonPrefixes();
        Assert.assertEquals( commonPrefixes.size(), 1 );
        Assert.assertEquals( commonPrefixes.get( 0 ),
                URLEncoder.encode( "aa/bb/key16481_3/", "UTF-8" ) );

        // test b : 非法值，取值为null
        ListVersionsRequest request2 = new ListVersionsRequest();
        try {
            s3Client.listVersions( request2 );
            Assert.fail( "exp fail but found true" );
        } catch ( IllegalArgumentException e ) {
            Assert.assertEquals( e.getMessage(),
                    "The bucket name parameter must be specified when listing versions in a bucket" );
        }
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                CommLib.clearBucket( s3Client, bucketName );
            }
        } catch ( BaseException e ) {
            Assert.fail( "clean up failed:" + e.getMessage() );
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }
}
