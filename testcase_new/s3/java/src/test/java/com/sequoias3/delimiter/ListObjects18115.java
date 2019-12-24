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
import java.util.Collections;
import java.util.List;

/**
 * @Description seqDB-18115: To get a list of objects within a bucket.specify
 *              matching delimiter and prefix,prefix not include delimiter
 * @author wuyan
 * @Date 2019.04.22
 * @version 1.00
 */
public class ListObjects18115 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket18115";
    private String delimiter = "?";
    private AmazonS3 s3Client = null;
    private String prefix = "dir1";
    private String[] keyList = { "dir1?test1_18115.png",
            "dir1??dir2??/dir3/test2_18115", "dir1?test3_18115", "dir1",
            "dir1/dir2?aa?cc?test5_18115", "dir1?dir2?aa?dd?test6_18115",
            "dir1_18115", "testdir1.txt_18115" };

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
                    "testcontext18115_" + i );
        }
    }

    private void listObjectsAndCheckResult() {
        List<String> matchPrefixList = new ArrayList<>();
        matchPrefixList.add( "dir1?" );
        matchPrefixList.add( "dir1/dir2?" );
        List<String> matchContentsList = new ArrayList<>();
        matchContentsList.add( "dir1_18115" );
        matchContentsList.add( "dir1" );

        ListObjectsV2Request request = new ListObjectsV2Request()
                .withBucketName( bucketName ).withEncodingType( "url" );
        request.withDelimiter( delimiter ).withPrefix( prefix );
        ListObjectsV2Result result = s3Client.listObjectsV2( request );
        List<String> commonPrefixes = result.getCommonPrefixes();
        Collections.sort( matchPrefixList );
        Assert.assertEquals( commonPrefixes, matchPrefixList,
                "actPrefixes:" + commonPrefixes.toString() + "\n ecpPrefixes:"
                        + matchPrefixList.toString() );

        // objects do not match delimiter are displayed in contents,num is 2
        List<String> actContentsList = new ArrayList<>();
        List<S3ObjectSummary> objects = result.getObjectSummaries();
        for ( S3ObjectSummary os : objects ) {
            String key = os.getKey();
            actContentsList.add( key );
        }

        // check the keyName
        Collections.sort( matchContentsList );
        Assert.assertEquals( actContentsList, matchContentsList,
                "actContentsList:" + actContentsList.toString()
                        + "matchContentList:" + matchContentsList.toString() );
    }
}
