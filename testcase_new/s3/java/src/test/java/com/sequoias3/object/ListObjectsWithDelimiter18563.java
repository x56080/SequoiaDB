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
 * @Description seqDB-18563: listObjectV1 with mathcing delimiter. list result
 *              does not match delimiter.
 * @author wuyan
 * @Date 2019.06.19
 * @version 1.00
 */
public class ListObjectsWithDelimiter18563 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket18563";
    private String key = "/aa/bb/object18563";
    private AmazonS3 s3Client = null;
    private int objectNums = 10;
    private String delimiter = "%";

    @BeforeClass
    private void setUp() throws IOException {
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
    }

    @Test
    public void testListObjects() throws IOException {
        List<String> keyList = putObjects();
        listObjectsAndCheckResult( keyList );
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

    private void listObjectsAndCheckResult( List<String> keyList )
            throws IOException {
        List<String> queryKeyList = new ArrayList<>();
        ListObjectsRequest request = new ListObjectsRequest()
                .withBucketName( bucketName );
        request.withDelimiter( delimiter );
        ObjectListing result = s3Client.listObjects( request );
        List<String> commonPrefixes = result.getCommonPrefixes();
        // matching delimiter displays only 0 record
        Assert.assertEquals( commonPrefixes.size(), 0 );

        // objects do not match delimiter are displayed in contents,num is 10
        List<S3ObjectSummary> objects = result.getObjectSummaries();
        int contentsNums = 10;
        Assert.assertEquals( objects.size(), contentsNums );
        for ( S3ObjectSummary os : objects ) {
            String key = os.getKey();
            queryKeyList.add( key );
        }

        // check the keyName
        Collections.sort( keyList );
        Assert.assertEquals( queryKeyList, keyList );
    }

    private List<String> putObjects() {
        List<String> keyList = new ArrayList<>();
        String keyName;
        for ( int i = 0; i < objectNums; i++ ) {
            keyName = key + "_" + i;
            s3Client.putObject( bucketName, keyName, "testcontent_" + keyName );
            keyList.add( keyName );
        }
        return keyList;
    }
}
