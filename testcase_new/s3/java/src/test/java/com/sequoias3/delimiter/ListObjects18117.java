package com.sequoias3.delimiter;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.ListObjectsV2Request;
import com.amazonaws.services.s3.model.ListObjectsV2Result;
import com.amazonaws.services.s3.model.S3ObjectSummary;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.DelimiterUtils;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/**
 * @Description seqDB-18117: To get a list of objects within a bucket.specify
 *              matching delimiter and prefix,all objects match delimiter.
 * @author wuyan
 * @Date 2019.04.23
 * @version 1.00
 */
public class ListObjects18117 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket18117";
    private String delimiter = "test";
    private AmazonS3 s3Client = null;
    private String prefix = "dir1";
    private String[] keyList = { "dir1/test1_18117.png",
            "dir1/dir2/dir3/test2_18117", "dir1/test3_18117",
            "dir1/dir2/aa/test4_18117", "dir1/dir2/aa/cc/test5_18113",
            "dir1/dir2/aa/test6_18117", "dir1/test/test18117.png" };

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
        listObjectsAndCheckResult();
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
                    "testcontext18117_" + i );
        }
    }

    private void listObjectsAndCheckResult() {
        List<String> matchPrefixList = ObjectUtils
                .getCommPrefixes( keyList, prefix, delimiter );
        ListObjectsV2Request request = new ListObjectsV2Request()
                .withBucketName( bucketName ).withEncodingType( "url" );
        request.withDelimiter( delimiter ).withPrefix( prefix );
        ListObjectsV2Result result = s3Client.listObjectsV2( request );
        List<String> commonPrefixes = result.getCommonPrefixes();
        Collections.sort( matchPrefixList );
        Assert.assertEquals( commonPrefixes, matchPrefixList,
                "actPrefixes:" + commonPrefixes.toString() + "\n ecpPrefixes:"
                        + matchPrefixList.toString() );

        // objects do not match delimiter are displayed in contents,num is 10
        List<String> actContentsList = new ArrayList<>();
        List<S3ObjectSummary> objects = result.getObjectSummaries();
        for ( S3ObjectSummary os : objects ) {
            String key = os.getKey();
            actContentsList.add( key );
        }

        // check the keyName
        List<String> matchContentsList = new ArrayList<>();
        Assert.assertEquals( actContentsList, matchContentsList );
    }
}
