package com.sequoias3.delimiter;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.ListObjectsV2Request;
import com.amazonaws.services.s3.model.ListObjectsV2Result;
import com.amazonaws.services.s3.model.S3ObjectSummary;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.DelimiterUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.ArrayList;
import java.util.List;

/**
 * @Description seqDB-18121: To get a list of objects within a bucket.specify
 *              matching delimiter /prefix/start-after,no match prefix .
 * @author wuyan
 * @Date 2019.04.23
 * @version 1.00
 */
public class ListObjects18121 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket18121";
    private String delimiter = "?";
    private AmazonS3 s3Client = null;
    private String prefix = "test?";
    private String[] keyList = { "dir1?test?test1_18121.png",
            "dir1?test2_18121", "dir1?dir2?aa?test3_18121", "test18121" };

    @BeforeClass
    private void setUp() {
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
    }

    @Test
    public void testCreateObject() throws Exception {
        DelimiterUtils.putBucketDelimiter( bucketName, delimiter );
        putObjects();

        String startAfter = keyList[ 1 ];
        listObjectsAndCheckResult( startAfter );
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

    private void putObjects() {
        for ( int i = 0; i < keyList.length; i++ ) {
            String subKeyName = keyList[ i ];
            s3Client.putObject( bucketName, subKeyName,
                    "testcontext18121_" + i );
        }
    }

    private void listObjectsAndCheckResult( String startAfter ) {
        int matchKeySize = 0;
        ListObjectsV2Request request = new ListObjectsV2Request()
                .withBucketName( bucketName ).withEncodingType( "url" );
        request.withDelimiter( delimiter ).withPrefix( prefix )
                .withStartAfter( startAfter );
        ListObjectsV2Result result = s3Client.listObjectsV2( request );
        List<String> commonPrefixes = result.getCommonPrefixes();
        Assert.assertEquals( commonPrefixes.size(), matchKeySize,
                "actPrefixes:" + commonPrefixes.toString() );

        // objects do not match delimiter are displayed in contents,num is 0
        List<String> actContentsList = new ArrayList<>();
        List<S3ObjectSummary> objects = result.getObjectSummaries();
        for ( S3ObjectSummary os : objects ) {
            String key = os.getKey();
            actContentsList.add( key );
        }
        Assert.assertEquals( actContentsList.size(), matchKeySize,
                "actContentList:" + actContentsList.toString() );
    }
}
