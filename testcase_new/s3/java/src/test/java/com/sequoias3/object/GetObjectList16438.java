package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.ListObjectsV2Request;
import com.amazonaws.services.s3.model.ListObjectsV2Result;
import com.amazonaws.services.s3.model.S3ObjectSummary;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/**
 * test content: 两次查询间隔时间超过上下文生命周期时间
 *
 * @author wangkexin
 * @Date 2018.11.21
 * @version 1.00
 */
@Test(groups = "contextlifecycleconf") public class GetObjectList16438
        extends S3TestBase {
    private String bucketName = "bucket16438";
    private String keyName = "/dir/dir";
    private List<String> expresultList = new ArrayList<String>( 10 );
    private int objectTotalNum = 10;
    private AmazonS3 s3Client = null;
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws IOException {
        s3Client = CommLib.buildS3Client();
        // create bucket
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );

        // put multiple objects
        for ( int i = 0; i < objectTotalNum; i++ ) {
            String currentKeyName = keyName + i + "/16438";
            s3Client.putObject( bucketName, currentKeyName,
                    "object_file16438" );
            expresultList.add( currentKeyName );
        }
    }

    @Test
    public void testGetObjectList() throws Exception {
        int keyCount = 5;
        // first query
        ListObjectsV2Request req = new ListObjectsV2Request()
                .withBucketName( bucketName ).withMaxKeys( keyCount );
        ListObjectsV2Result result = s3Client.listObjectsV2( req );
        List<S3ObjectSummary> objectSummaries = result.getObjectSummaries();
        checkListObjectsV2Result( objectSummaries, keyCount );
        String NextContinuationToken = result.getNextContinuationToken();

        Thread.sleep( 3 * 60 * 1000 );
        // second query
        ListObjectsV2Request req2 = new ListObjectsV2Request()
                .withBucketName( bucketName )
                .withContinuationToken( NextContinuationToken );
        try {
            s3Client.listObjectsV2( req2 );
            Assert.fail( "exp fail but found success" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "InvalidArgument" );
        }

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        if ( runSuccess ) {
            CommLib.deleteAllObjectVersions( s3Client, bucketName );
            s3Client.deleteBucket( bucketName );
        }
    }

    private void checkListObjectsV2Result(
            List<S3ObjectSummary> objectSummaries, int expCount ) {
        Assert.assertEquals( objectSummaries.size(), expCount,
                "The number of returned results is wrong" );
        Collections.sort( expresultList );
        for ( int i = 0; i < objectSummaries.size(); i++ ) {
            Assert.assertEquals( objectSummaries.get( i ).getKey(),
                    expresultList.get( i ), "commonPrefixes is wrong" );
        }
    }
}
