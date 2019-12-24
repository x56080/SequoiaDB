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
 * @Description seqDB-18114: To get a list of objects within a bucket.specify
 *              matching delimiter and prefix,prefix matching empty dir(match
 *              deleteTag object)
 * @author wuyan
 * @Date 2019.04.22
 * @version 1.00
 */
public class ListObjects18114 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket18114";
    private String delimiter = "?";
    private AmazonS3 s3Client = null;
    private String prefix = "dir1";
    private String[] keyList = { "dir1?test1_18114.png",
            "dir1??dir2??/dir3/test2_18114", "dir1?test3_18114", "dir1/18114",
            "dir1/dir2?aa?cc?test5_18114", "dir1?dir2?aa?dd?test6_18114",
            "dir1_18114", "dir1_?test18114", "testdir1.txt_18114" };

    @BeforeClass
    private void setUp() {
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );
    }

    @Test
    public void testCreateObject() throws Exception {
        DelimiterUtils.putBucketDelimiter( bucketName, delimiter );
        putObjectsAndDeleteTagObjects();
        listObjectsAndCheckResult();
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

    private void putObjectsAndDeleteTagObjects() {
        for ( int i = 0; i < keyList.length; i++ ) {
            if ( i % 2 == 0 ) {
                // put objects
                String subKeyName = keyList[ i ];
                s3Client.putObject( bucketName, subKeyName,
                        "testcontext18114_" + i );
            } else {
                // put DeleteTag
                // Object:dir1??dir2??/dir3/test2_18114,dir1/18114,dir1?dir2?aa?dd?test6_18114,dir1_?test18114
                String subKeyName = keyList[ i ];
                s3Client.deleteObject( bucketName, subKeyName );
            }
        }
    }

    private void listObjectsAndCheckResult() {
        List<String> matchPrefixList = new ArrayList<>();
        matchPrefixList.add( "dir1?" );
        matchPrefixList.add( "dir1/dir2?" );
        // match deleteTag perfix:dir1_?
        matchPrefixList.add( "dir1_?" );
        List<String> matchContentsList = new ArrayList<>();
        matchContentsList.add( "dir1_18114" );

        ListObjectsV2Request request = new ListObjectsV2Request()
                .withBucketName( bucketName ).withEncodingType( "url" );
        request.withDelimiter( delimiter ).withPrefix( prefix );
        ListObjectsV2Result result = s3Client.listObjectsV2( request );
        List<String> commonPrefixes = result.getCommonPrefixes();
        Collections.sort( matchPrefixList );
        Assert.assertEquals( commonPrefixes, matchPrefixList,
                "actPrefixes:" + commonPrefixes.toString() + "\n ecpPrefixes:"
                        + matchPrefixList.toString() );

        // objects do not match delimiter are displayed in contents,num is 1
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
