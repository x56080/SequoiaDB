package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.ListObjectsV2Request;
import com.amazonaws.services.s3.model.ListObjectsV2Result;
import com.sequoiadb.exception.BaseException;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.List;

/**
 * test content: GetBucketVersion2接口参数校验 testlink-case: seqDB-16480
 *
 * @author wangkexin
 * @Date 2019.01.08
 * @version 1.00
 */
public class TestGetBucketVersion2_16480 extends S3TestBase {
    private String bucketName = "bucket16480";
    private AmazonS3 s3Client = null;
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );
        s3Client.putObject( bucketName, "aa/bb/key16480_1/123",
                "content16480_1" );
        s3Client.putObject( bucketName, "aa/bb/key16480_2/456",
                "content16480_2" );
    }

    @Test
    public void testListObjectsV2() {
        // test a : 合法值
        ListObjectsV2Request request = new ListObjectsV2Request()
                .withBucketName( bucketName ).withEncodingType( "url" );
        request.withMaxKeys( 1 );
        request.withPrefix( "aa/bb/" );
        request.withDelimiter( "/" );
        request.withFetchOwner( true );
        request.withStartAfter( "aa" );
        ListObjectsV2Result result = s3Client.listObjectsV2( request );

        List< String > commonPrefixes = result.getCommonPrefixes();
        Assert.assertEquals( commonPrefixes.size(), 1 );
        Assert.assertEquals( commonPrefixes.get( 0 ), "aa/bb/key16480_1/" );

        // test b : 非法值，取值为null
        ListObjectsV2Request request2 = new ListObjectsV2Request();
        try {
            s3Client.listObjectsV2( request2 );
            Assert.fail( "exp fail but found true" );
        } catch ( IllegalArgumentException e ) {
            Assert.assertEquals( e.getMessage(),
                    "The bucket name parameter must be specified when listing objects in a bucket" );
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
