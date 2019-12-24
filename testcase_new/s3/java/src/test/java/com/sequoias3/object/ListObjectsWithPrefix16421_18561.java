package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.ListObjectsRequest;
import com.amazonaws.services.s3.model.ListObjectsV2Request;
import com.amazonaws.services.s3.model.ListObjectsV2Result;
import com.amazonaws.services.s3.model.ObjectListing;
import com.amazonaws.services.s3.model.S3ObjectSummary;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.IOException;
import java.util.List;

/**
 * @Description seqDB-16421: To get a list by listObjectV2 within a
 *              bucket.specify prefix does not match the object.seqDB-18561: To
 *              get a list by listObjectV1 within a bucket.specify prefix does
 *              not match the object.
 * @author wuyan
 * @Date 2018.11.19
 * @version 1.00
 */
public class ListObjectsWithPrefix16421_18561 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket16421";
    private String key = "/aa/bb/object16421";
    private AmazonS3 s3Client = null;
    private int matchObjectNums = 0;
    private String prefix = "/dir_1/prefix/test16421";
    ;

    @BeforeClass
    private void setUp() {
        s3Client = CommLib.buildS3Client();
        s3Client.createBucket( bucketName );
    }

    @Test
    public void testCreateObject() throws Exception {
        putObjects();
        listObjectsAndCheckResult();
        listObjectV1AndCheckResult();
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

    private void listObjectsAndCheckResult() throws IOException {
        ListObjectsV2Request request = new ListObjectsV2Request()
                .withBucketName( bucketName ).withEncodingType( "url" );
        request.withPrefix( prefix );
        ListObjectsV2Result result = s3Client.listObjectsV2( request );
        List<S3ObjectSummary> objects = result.getObjectSummaries();
        Assert.assertEquals( objects.size(), matchObjectNums );
    }

    private void listObjectV1AndCheckResult() throws IOException {
        ListObjectsRequest request = new ListObjectsRequest()
                .withBucketName( bucketName );
        request.withPrefix( prefix );
        ObjectListing result = s3Client.listObjects( request );
        List<S3ObjectSummary> objects = result.getObjectSummaries();
        Assert.assertEquals( objects.size(), matchObjectNums );
    }

    private void putObjects() {
        int objectNums = 10;
        for ( int i = 0; i < objectNums; i++ ) {
            String keyName = key + "_" + i + TestTools.getRandomString( i );
            s3Client.putObject( bucketName, keyName, "testContext" );
        }
    }
}
