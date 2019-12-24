package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
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
 * @Description seqDB-18569: To get a list by listObjectV1.specify
 *              marker/prefix/delimiter. match delimiter and marker, no match
 *              prefix; seqDB-18570: To get a list by listObjectV1.specify match
 *              marker/prefix/delimiter.
 * @author wuyan
 * @Date 2019.06.20
 * @version 1.00
 */
public class ListObjectsWithMarkerAndPrefix18569_18570 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket18569";
    private String key = "/object18569.png";
    private AmazonS3 s3Client = null;
    private int objectNums = 10;
    private String delimiter = "%";
    private List<String> matchKeyList = new ArrayList<>();

    @BeforeClass
    private void setUp() {
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );
        DelimiterUtils.putBucketDelimiter( bucketName, delimiter );
        putObjects();
    }

    @Test
    public void testListObjects() {
        // test 18569: no match prefix
        String marker = "/dir/";
        String prefixA = "test18569";
        listObjectsAndCheckResultA( marker, prefixA, delimiter );

        // test 18570: no match delimiter
        int startPosition = 1;
        String prefixB = "/dir/";
        String delimiterB = "/a";
        listObjectsAndCheckResultB( startPosition, prefixB, delimiterB );

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

    // no mathch prefix
    private void listObjectsAndCheckResultA( String matchMarker,
            String matchPrefix, String matchDelimiter ) {
        ListObjectsRequest request = new ListObjectsRequest()
                .withBucketName( bucketName ).withMarker( matchMarker )
                .withPrefix( matchPrefix ).withDelimiter( matchDelimiter );
        ObjectListing result = s3Client.listObjects( request );
        List<String> actCommonPrefixes = result.getCommonPrefixes();
        // no match key,the commonprefixes is 0
        Assert.assertEquals( actCommonPrefixes.size(), 0 );

        List<String> queryKeyList = new ArrayList<>();
        List<S3ObjectSummary> objects = result.getObjectSummaries();
        for ( S3ObjectSummary os : objects ) {
            String key = os.getKey();
            queryKeyList.add( key );
        }

        Assert.assertEquals( queryKeyList.size(), 0,
                "queryKey=" + queryKeyList.toString() );
    }

    // no match delimiter
    private void listObjectsAndCheckResultB( int startPosition,
            String matchPrefix, String matchDelimiter ) {
        Collections.sort( matchKeyList );
        String matchMarker = matchKeyList.get( startPosition );
        ListObjectsRequest request = new ListObjectsRequest()
                .withBucketName( bucketName ).withMarker( matchMarker )
                .withPrefix( matchPrefix ).withDelimiter( matchDelimiter );
        ObjectListing result = s3Client.listObjects( request );
        List<String> actCommonPrefixes = result.getCommonPrefixes();
        // no match key,the commonprefixes is 0
        Assert.assertEquals( actCommonPrefixes.size(), 0 );

        List<String> queryKeyList = new ArrayList<>();
        List<S3ObjectSummary> objects = result.getObjectSummaries();
        for ( S3ObjectSummary os : objects ) {
            String key = os.getKey();
            queryKeyList.add( key );
        }

        // check the keyName
        for ( int i = 0; i < startPosition + 1; i++ ) {
            matchKeyList.remove( 0 );
        }
        Assert.assertEquals( queryKeyList, matchKeyList,
                "queryKey=" + queryKeyList.toString() + "\nexpKeys:"
                        + matchKeyList.toString() );
    }

    private void putObjects() {
        String keyName;
        for ( int i = 0; i < objectNums; i++ ) {
            if ( i % 2 == 0 ) {
                keyName = key + "_" + i;
            } else {
                String prefix = "/dir/" + i;
                // the key include prefix and delimiter
                keyName = prefix + "_" + delimiter + "_" + key;
                matchKeyList.add( keyName );
            }
            s3Client.putObject( bucketName, keyName, "test18569" + i );
        }
    }
}
