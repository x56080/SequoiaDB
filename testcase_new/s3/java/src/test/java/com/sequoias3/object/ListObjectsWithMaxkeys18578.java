package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.ListObjectsRequest;
import com.amazonaws.services.s3.model.ObjectListing;
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
 * @Description seqDB-18578: To get a list by listObjectV1.specify matching
 *              maxKeys list query mulitiple times.
 * @author wuyan
 * @Date 2019.06.21
 * @version 1.00
 */
public class ListObjectsWithMaxkeys18578 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket18578";
    private String key = "aa%%bb%object18578.png";
    private AmazonS3 s3Client = null;
    private int objectNums = 20;

    @BeforeClass
    private void setUp() throws IOException {
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
    }

    @Test
    public void testListObjects() throws Exception {

        // test a: setMarker is the last key
        List<String> keyListA = putObjects();
        listObjectsAndCheckResultA( keyListA );
        deleteObjects( keyListA );

        // test b: setMarker not match key
        List<String> keyListB = putObjects();
        listObjectsAndCheckResultB( keyListB );
        deleteObjects( keyListB );

        // test c: setMarker not match key
        List<String> keyListC = putObjects();
        listObjectsAndCheckResultC( keyListC );
        deleteObjects( keyListC );

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

    private void listObjectsAndCheckResultA( List<String> keyList ) {
        int maxKeys = 5;
        ListObjectsRequest request = new ListObjectsRequest()
                .withBucketName( bucketName ).withMaxKeys( maxKeys );
        ObjectListing result;
        List<String> queryKeyList = new ArrayList<>();
        do {
            result = s3Client.listObjects( request );
            List<S3ObjectSummary> objects = result.getObjectSummaries();
            List<String> oneQueryKeyList = new ArrayList<>();
            String key = "";
            for ( S3ObjectSummary os : objects ) {
                key = os.getKey();
                oneQueryKeyList.add( key );
                queryKeyList.add( key );
            }

            if ( oneQueryKeyList.size() > maxKeys ) {
                Assert.fail( "query list key nums error! oneQueryKeys :"
                        + oneQueryKeyList.toString() );
            }

            // delete the last key
            s3Client.deleteObject( bucketName, key );
            request.setMarker( key );
        } while ( result.isTruncated() );

        // check the keyName
        Assert.assertEquals( queryKeyList, keyList );
    }

    private void listObjectsAndCheckResultB( List<String> keyList ) {
        int maxKeys = 6;
        ListObjectsRequest request = new ListObjectsRequest()
                .withBucketName( bucketName ).withMaxKeys( maxKeys );
        ObjectListing result;
        List<String> expKeyList = new ArrayList<>( keyList );
        List<String> expfirstQueryKeyList = expKeyList.subList( 0, maxKeys );
        String marker = keyList.get( keyList.size() - 1 );
        // first list
        result = s3Client.listObjects( request );
        List<S3ObjectSummary> objects = result.getObjectSummaries();
        List<String> oneQueryKeyList = new ArrayList<>();
        String key = "";
        for ( S3ObjectSummary os : objects ) {
            key = os.getKey();
            oneQueryKeyList.add( key );
        }
        Assert.assertEquals( oneQueryKeyList, expfirstQueryKeyList,
                "first query keys:" + oneQueryKeyList.toString() + "\n expKeys:"
                        + expfirstQueryKeyList.toString() );
        Assert.assertTrue( result.isTruncated() );

        // delete the last key, set the marker not match key
        s3Client.deleteObject( bucketName, key );
        request.setMarker( marker );
        // second list, not match key
        result = s3Client.listObjects( request );
        List<S3ObjectSummary> objects1 = result.getObjectSummaries();
        List<String> secondQueryKeyList = new ArrayList<>();
        for ( S3ObjectSummary os : objects1 ) {
            String key1 = os.getKey();
            secondQueryKeyList.add( key1 );
        }
        Assert.assertEquals( secondQueryKeyList.size(), 0,
                "list keys:" + secondQueryKeyList.toString() );
        Assert.assertFalse( result.isTruncated() );
    }

    private void listObjectsAndCheckResultC( List<String> keyList ) {
        int maxKeys = 12;
        ListObjectsRequest request = new ListObjectsRequest()
                .withBucketName( bucketName ).withMaxKeys( maxKeys );
        ObjectListing result;
        List<String> expKeyList = new ArrayList<>( keyList );
        List<String> expfirstQueryKeyList = expKeyList.subList( 0, maxKeys );

        // first list
        result = s3Client.listObjects( request );
        List<S3ObjectSummary> objects = result.getObjectSummaries();
        List<String> oneQueryKeyList = new ArrayList<>();

        for ( S3ObjectSummary os : objects ) {
            String key = os.getKey();
            oneQueryKeyList.add( key );
        }
        Assert.assertEquals( oneQueryKeyList, expfirstQueryKeyList,
                "first query keys:" + oneQueryKeyList.toString() + "\n expKeys:"
                        + expfirstQueryKeyList.toString() );
        Assert.assertTrue( result.isTruncated() );

        // delete the last key
        s3Client.deleteObject( bucketName, key );
        // set marker to match the key of the first query,eg:
        // oneQueryKeyList.get(10)
        int serialNo = 10;
        String marker = oneQueryKeyList.get( serialNo );
        request.setMarker( marker );
        result = s3Client.listObjects( request );
        List<S3ObjectSummary> objects1 = result.getObjectSummaries();
        List<String> secondQueryKeyList = new ArrayList<>();
        for ( S3ObjectSummary os : objects1 ) {
            String key1 = os.getKey();
            secondQueryKeyList.add( key1 );
        }
        List<String> expSecondQueryKeyList = expKeyList
                .subList( serialNo + 1, expKeyList.size() );
        Assert.assertEquals( secondQueryKeyList, expSecondQueryKeyList,
                "second list keys:" + secondQueryKeyList.toString()
                        + "\n expListkeys:" + expSecondQueryKeyList.toString()
                        + "\n match marker is :" + marker );
        Assert.assertFalse( result.isTruncated() );
    }

    private List<String> putObjects() {
        List<String> keyList = new ArrayList<>();
        for ( int i = 0; i < objectNums; i++ ) {
            String keyName = key + "_" + i;
            keyList.add( keyName );
            s3Client.putObject( bucketName, keyName, "test18564" + i );
        }
        Collections.sort( keyList );
        return keyList;
    }

    private void deleteObjects( List<String> keyList ) {
        for ( String keyName : keyList ) {
            s3Client.deleteObject( bucketName, keyName );
        }
    }
}
