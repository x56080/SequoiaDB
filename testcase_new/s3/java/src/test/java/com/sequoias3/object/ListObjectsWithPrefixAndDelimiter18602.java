package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.ListObjectsRequest;
import com.amazonaws.services.s3.model.ObjectListing;
import com.amazonaws.services.s3.model.S3ObjectSummary;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.ArrayList;
import java.util.List;

/**
 * @Description seqDB-18602: To get a list by listObjectV1.specify
 *              prefix/delimiter.no matching prefix.
 * @author wuyan
 * @Date 2019.06.20
 * @version 1.00
 */

public class ListObjectsWithPrefixAndDelimiter18602 extends S3TestBase {
    private String bucketName = "bucket18602";
    private String[] keyList = { "dir/atest?_18602.png",
            "dir/test/test?_18602.png", "dir1/dir2?%test_18602.png",
            "dir1/dir2?test_18602", "dir1_18602" };
    private AmazonS3 s3Client = null;
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() {
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );

        for ( int i = 0; i < keyList.length; i++ ) {
            String subKeyName = keyList[ i ];
            s3Client.putObject( bucketName, subKeyName,
                    "testcontext18575_" + i );
        }
    }

    @Test
    private void testListObjects() {
        List<String> matchPrefixList = new ArrayList<>();
        List<String> matchContentsList = new ArrayList<>();

        String delimiter1 = "?";
        String prefix = "dir1?";
        ListObjectsRequest request = new ListObjectsRequest()
                .withBucketName( bucketName );
        request.withDelimiter( delimiter1 ).withPrefix( prefix );
        ObjectListing result = s3Client.listObjects( request );
        List<String> commonPrefixes = result.getCommonPrefixes();
        Assert.assertEquals( commonPrefixes, matchPrefixList,
                "actPrefixes:" + commonPrefixes.toString() + "\n expPrefixes:"
                        + matchPrefixList.toString() );

        List<String> actContentsList = new ArrayList<>();
        List<S3ObjectSummary> objects = result.getObjectSummaries();
        for ( S3ObjectSummary os : objects ) {
            String key = os.getKey();
            actContentsList.add( key );
        }
        Assert.assertEquals( actContentsList, matchContentsList,
                "actContents:" + actContentsList.toString() + "\n expContents:"
                        + matchContentsList.toString() );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                for ( int i = 0; i < keyList.length; i++ ) {
                    String keyName = keyList[ i ];
                    s3Client.deleteObject( bucketName, keyName );
                }
                s3Client.deleteBucket( bucketName );
            }
        } finally {
            s3Client.shutdown();
        }
    }
}
