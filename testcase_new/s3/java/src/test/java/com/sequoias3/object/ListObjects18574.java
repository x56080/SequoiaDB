package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.ListObjectsRequest;
import com.amazonaws.services.s3.model.ObjectListing;
import com.amazonaws.services.s3.model.S3ObjectSummary;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.DelimiterUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/**
 * @Description seqDB-18574: To get a list by listObjectV1.specify
 *              marker/prefix/delimiter/maxkeys.
 * @author wuyan
 * @Date 2019.06.20
 * @version 1.00
 */

public class ListObjects18574 extends S3TestBase {
    private String bucketName = "bucket18574";
    private String prefix = "dir1/";
    private String delimiter = "?";
    private String[] keyNames = { "a/test0_18574", "a/test1_18574",
            "dir1/atest2_18574.png", "dir1/dir2/aa?dd/test3_18574",
            "dir1/dir2/dir3/test?4_18574", "dir1/dir2/xx?test5_18574",
            "dir1/test6_18574", "dir1/test?7_18574", "dir1/test?8_18574",
            "dir1/test/aa9_18574", "fdir1/test10?_18574",
            "testdir1?11.txt_18574" };
    private AmazonS3 s3Client = null;
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() {
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );
        DelimiterUtils.putBucketDelimiter( bucketName, delimiter );
        for ( int i = 0; i < keyNames.length; i++ ) {
            s3Client.putObject( bucketName, keyNames[ i ],
                    "testContest_" + keyNames[ i ] );
        }
    }

    @Test
    public void testListObjects() throws Exception {
        // test a: match nums < maxkeys
        int maxKeysA = 10;
        listObjectsAndCheckResult( prefix, delimiter, maxKeysA );

        // test b: match nums > maxkeys
        int maxKeysB = 2;
        listObjectsAndCheckResult( prefix, delimiter, maxKeysB );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                CommLib.deleteAllObjectVersions( s3Client, bucketName );
                s3Client.deleteBucket( bucketName );
            }
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }

    private void listObjectsAndCheckResult( String prefix, String delimiter,
            int maxKeys ) {
        String marker = "dir1/dir2/aa?";
        ListObjectsRequest request = new ListObjectsRequest()
                .withBucketName( bucketName ).withMarker( marker )
                .withPrefix( prefix ).withDelimiter( delimiter )
                .withMaxKeys( maxKeys );
        ObjectListing result;
        List<String> commonPrefixes = new ArrayList<>();
        List<String> queryKeyList = new ArrayList<>();
        do {
            result = s3Client.listObjects( request );
            List<String> oneGetCommPrefixes = result.getCommonPrefixes();
            commonPrefixes.addAll( oneGetCommPrefixes );
            List<S3ObjectSummary> objects = result.getObjectSummaries();
            List<String> oneQueryKeyList = new ArrayList<>();
            for ( S3ObjectSummary os : objects ) {
                String key = os.getKey();
                oneQueryKeyList.add( key );
                queryKeyList.add( key );
            }
            String nextMarker = result.getNextMarker();
            request.setMarker( nextMarker );

            int eachListNums =
                    oneGetCommPrefixes.size() + oneQueryKeyList.size();
            if ( eachListNums > maxKeys ) {
                Assert.fail(
                        "list nums error! commonPrefixes: " + oneGetCommPrefixes
                                .toString() + "  contents:" + oneQueryKeyList
                                .toString() + "\n eachListNums=" + eachListNums
                                + "  maxKeys=" + maxKeys );
            }
        } while ( result.isTruncated() );

        // check the commprefixList and contents
        List<String> expCommprefixLists = new ArrayList<>();
        expCommprefixLists.add( "dir1/dir2/dir3/test?" );
        expCommprefixLists.add( "dir1/test?" );
        expCommprefixLists.add( "dir1/dir2/xx?" );

        List<String> expContentLists = new ArrayList<>();
        expContentLists.add( "dir1/test6_18574" );
        expContentLists.add( "dir1/test/aa9_18574" );

        Collections.sort( expCommprefixLists );
        Assert.assertEquals( commonPrefixes, expCommprefixLists,
                "commonPrefixes:" + commonPrefixes.toString()
                        + "\n expCommprefixList:" + expCommprefixLists
                        .toString() );
        Collections.sort( expContentLists );
        Assert.assertEquals( queryKeyList, expContentLists,
                "matchContents:" + queryKeyList.toString()
                        + "\n expContentList:" + expContentLists.toString() );

    }
}
