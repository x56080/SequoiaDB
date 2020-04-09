package com.sequoias3.object;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.ListObjectsRequest;
import com.amazonaws.services.s3.model.ObjectListing;
import com.amazonaws.services.s3.model.S3ObjectSummary;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;

/**
 * @Description seqDB-18577: To get a list by listObjectV1.Delete nextMarker
 *              matching record.
 * @author wuyan
 * @Date 2019.06.20
 * @version 1.00
 */
public class ListObjects18577 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket18577";
    private String key = "%aa%%bb%object18577.png";
    private AmazonS3 s3Client = null;
    private int objectNums = 10;

    @BeforeClass
    private void setUp() {
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
    }

    @Test
    public void testListObjects() throws Exception {
        List< String > keyList = putObjects();
        // test a: maxkeys < objectNums
        int maxKeysA = 3;
        listObjectsAndCheckResult( keyList, maxKeysA );

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

    private void listObjectsAndCheckResult( List< String > keyList,
            int maxKeys ) throws IOException {
        ListObjectsRequest request = new ListObjectsRequest()
                .withBucketName( bucketName ).withMaxKeys( maxKeys );
        ObjectListing result;
        List< String > queryKeyList = new ArrayList<>();
        do {
            result = s3Client.listObjects( request );
            List< S3ObjectSummary > objects = result.getObjectSummaries();

            List< String > oneQueryKeyList = new ArrayList<>();
            for ( S3ObjectSummary os : objects ) {
                String key = os.getKey();
                queryKeyList.add( key );
                oneQueryKeyList.add( key );
            }

            String nextMarker = result.getNextMarker();
            // last query does not return nextMarker, the nextMarker is ""
            if ( nextMarker != null ) {
                s3Client.deleteObject( bucketName, nextMarker );
            }
            request.setMarker( nextMarker );
        } while ( result.isTruncated() );

        // check the keyName
        Collections.sort( keyList );
        Assert.assertEquals( queryKeyList, keyList,
                "queryKeys:" + queryKeyList.toString() + "\nexpQueryKeys:"
                        + keyList.toString() );
    }

    private List< String > putObjects() {
        List< String > keyList = new ArrayList<>();
        for ( int i = 0; i < objectNums; i++ ) {
            String keyName = key + "_" + i;
            keyList.add( keyName );
            s3Client.putObject( bucketName, keyName, "test18577" + i );
        }
        return keyList;
    }
}
