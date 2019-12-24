package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
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
 * @Description seqDB-16431: To get a list of objects within a bucket.specify
 *              startAfter/prefix/delimiter. match prefix and startAfter, no
 *              match delimiter
 * @author wuyan
 * @Date 2018.11.27
 * @version 1.00
 */
public class ListObjectsWithStartAfterAndPrefix16431 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket16431";
    private String key = "%aa%%bb%object16431.png";
    private AmazonS3 s3Client = null;
    private int objectNums = 100;
    private String prefix = "%dir_1%prefix%test16431";

    @BeforeClass
    private void setUp() {
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );
    }

    @Test
    public void testListObjects() throws Exception {
        List<String> keyList = putObjects();

        int startAfterNo = 20;
        String delimiter = "delimiter16431";
        listObjectsAndCheckResult( keyList, startAfterNo, delimiter );

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
            int startAfterNo, String delimiter ) throws IOException {
        List<String> expKeyList = new ArrayList<>( keyList );
        Collections.sort( expKeyList );
        ListObjectsV2Request request = new ListObjectsV2Request()
                .withBucketName( bucketName ).withEncodingType( "url" )
                .withStartAfter( expKeyList.get( startAfterNo ) )
                .withPrefix( prefix ).withDelimiter( delimiter );
        ListObjectsV2Result result;
        List<String> queryKeyList = new ArrayList<>();
        do {
            result = s3Client.listObjectsV2( request );
            List<S3ObjectSummary> objects = result.getObjectSummaries();
            for ( S3ObjectSummary os : objects ) {
                String key = os.getKey();
                queryKeyList.add( key );
            }

            String continuationToken = result.getNextContinuationToken();
            request.setContinuationToken( continuationToken );
        } while ( result.isTruncated() );

        // check the keyName
        Collections.sort( queryKeyList );
        for ( int i = 0; i < startAfterNo + 1; i++ ) {
            expKeyList.remove( 0 );
        }
        Assert.assertEquals( queryKeyList, expKeyList );
    }

    private List<String> putObjects() {
        List<String> matchKeyList = new ArrayList<>();
        String keyName;
        for ( int i = 0; i < objectNums; i++ ) {
            if ( i % 2 == 0 ) {
                keyName = key + "_" + i;
            } else {
                keyName = prefix + "_" + i;
                matchKeyList.add( keyName );
            }
            s3Client.putObject( bucketName, keyName, "test16431" + i );
            s3Client.putObject( bucketName, keyName,
                    "test16431updateContent" + i );
        }
        return matchKeyList;
    }
}
