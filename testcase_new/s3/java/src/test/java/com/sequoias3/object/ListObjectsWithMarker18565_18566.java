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
 * @Description seqDB-18565: To get a list by listObjectV1.specify matching
 *              marker.seqDB-18566: To get a list by listObjectV1.specify no
 *              matching marker.
 * @author wuyan
 * @Date 2019.06.19
 * @version 1.00
 */
public class ListObjectsWithMarker18565_18566 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket18565";
    private String key = "/aa//bb/object18565.png";
    private AmazonS3 s3Client = null;
    private int objectNums = 200;

    @BeforeClass
    private void setUp() {
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
    }

    @Test
    public void testListObjects() throws Exception {
        List<String> keyList = putObjects();
        // test a: starting with the first record
        int startPositionA = 0;
        listObjectsAndCheckResult( keyList, startPositionA );

        // test b: starting in the middle
        int startPositionB = objectNums / 2;
        listObjectsAndCheckResult( keyList, startPositionB );

        // test c:starting with the last one
        int startPositionC = objectNums - 1;
        listObjectsAndCheckResult( keyList, startPositionC );

        // test d: show only the last object
        int startPositionD = objectNums - 2;
        listObjectsAndCheckResult( keyList, startPositionD );

        // test 18566: mismatch the object
        String marker = "/aa//bb/test18565.txt";
        misMatchObject( marker );
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

    private void listObjectsAndCheckResult( List<String> keyList,
            int startPosition ) throws IOException {
        List<String> expKeyList = new ArrayList<>( keyList );
        String marker = expKeyList.get( startPosition );
        ListObjectsRequest request = new ListObjectsRequest()
                .withBucketName( bucketName ).withMarker( marker );
        ObjectListing result;
        List<String> queryKeyList = new ArrayList<>();
        do {
            result = s3Client.listObjects( request );
            List<S3ObjectSummary> objects = result.getObjectSummaries();
            for ( S3ObjectSummary os : objects ) {
                String key = os.getKey();
                queryKeyList.add( key );
            }
            String nextMarker = result.getNextMarker();
            request.setMarker( nextMarker );
        } while ( result.isTruncated() );

        // check the keyName
        expKeyList.subList( 0, startPosition + 1 ).clear();
        Assert.assertEquals( queryKeyList, expKeyList );
    }

    private void misMatchObject( String marker ) {
        ListObjectsRequest request = new ListObjectsRequest()
                .withBucketName( bucketName ).withMarker( marker );
        ObjectListing result = s3Client.listObjects( request );
        List<S3ObjectSummary> objects = result.getObjectSummaries();
        // misMatchObject, the list size is 0
        Assert.assertEquals( objects.size(), 0 );
    }

    private List<String> putObjects() {
        List<String> keyList = new ArrayList<>();
        for ( int i = 0; i < objectNums; i++ ) {
            String keyName = key + "_" + i;
            s3Client.putObject( bucketName, keyName, "test18565" + i );
            keyList.add( keyName );
        }
        Collections.sort( keyList );
        return keyList;
    }
}
