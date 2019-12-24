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
 * @Description seqDB-18564: To get a list by listObjectV1.specify matching
 *              maxKeys.
 * @author wuyan
 * @Date 2019.06.19
 * @version 1.00
 */
public class ListObjectsWithMaxkeys18564 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket18564";
    private String key = "%aa%%bb%object18564.png";
    private AmazonS3 s3Client = null;
    private int objectNums = 50;

    @BeforeClass
    private void setUp() throws IOException {
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
    }

    @Test
    public void testListObjects() throws Exception {
        List<String> keyList = putObjects();
        // test a: maxkeys < objectNums
        int maxKeysA = 5;
        listObjectsAndCheckResult( keyList, maxKeysA );

        // test b: maxKeys > objectNums
        int maxKeysB = objectNums + 1;
        listObjectsAndCheckResult( keyList, maxKeysB );

        // test c: maxKeys = objectNums
        int maxKeysC = objectNums;
        listObjectsAndCheckResult( keyList, maxKeysC );

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

    private void listObjectsAndCheckResult( List<String> keyList, int maxKeys )
            throws IOException {
        ListObjectsRequest request = new ListObjectsRequest()
                .withBucketName( bucketName ).withMaxKeys( maxKeys );
        ObjectListing result;
        List<String> queryKeyList = new ArrayList<>();
        do {
            result = s3Client.listObjects( request );
            List<S3ObjectSummary> objects = result.getObjectSummaries();
            int oneQueryKeyNums = 0;
            for ( S3ObjectSummary os : objects ) {
                String key = os.getKey();
                queryKeyList.add( key );
                oneQueryKeyNums++;
            }

            if ( maxKeys < objectNums ) {
                Assert.assertEquals( oneQueryKeyNums, maxKeys );
            } else {
                Assert.assertEquals( oneQueryKeyNums, objectNums );
            }
            String nextMarker = result.getNextMarker();
            request.setMarker( nextMarker );
        } while ( result.isTruncated() );

        // check the keyName
        Collections.sort( keyList );
        Assert.assertEquals( queryKeyList, keyList );
    }

    private List<String> putObjects() {
        List<String> keyList = new ArrayList<>();
        for ( int i = 0; i < objectNums; i++ ) {
            String keyName = key + "_" + i;
            keyList.add( keyName );
            s3Client.putObject( bucketName, keyName, "test18564" + i );
        }
        return keyList;
    }
}
