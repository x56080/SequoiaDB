package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.ListObjectsRequest;
import com.amazonaws.services.s3.model.ObjectListing;
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
 * @Description seqDB-18571: To get a list by listObjectV1.specify matching
 *              delimiter and prefix,the object name include delimiter.
 * @author wuyan
 * @Date 2019.06.20
 * @version 1.00
 */
public class ListObjectsWithPrefixAndDelimiter18571 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket18571";
    private String delimiter = "?";
    private AmazonS3 s3Client = null;
    private String prefix = "dir1?";
    private String[] keyList = { "dir1?test1_18571.png",
            "dir1??dir2??/dir3/test2_18571", "dir1?test3_18571",
            "dir1?dir2?aa?test4_18571", "dir1?dir2?aa?cc?test5_18571",
            "dir1?dir2?aa?dd?test6_18571", "dir1_18571", "testdir1.txt_18571" };

    @BeforeClass
    private void setUp() {
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
    }

    @Test
    public void testListObjects() throws Exception {
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
                    "testcontext18571_" + i );
        }
    }

    private void listObjectsAndCheckResult() {
        List<String> matchPrefixList = new ArrayList<>();
        matchPrefixList.add( "dir1?dir2?" );
        matchPrefixList.add( "dir1??" );
        List<String> matchContentsList = new ArrayList<>();
        matchContentsList.add( "dir1?test1_18571.png" );
        matchContentsList.add( "dir1?test3_18571" );

        ListObjectsRequest request = new ListObjectsRequest()
                .withBucketName( bucketName );
        request.withDelimiter( delimiter ).withPrefix( prefix );
        ObjectListing result = s3Client.listObjects( request );
        List<String> commonPrefixes = result.getCommonPrefixes();
        Collections.sort( matchPrefixList );
        Assert.assertEquals( commonPrefixes, matchPrefixList,
                "actPrefixes:" + commonPrefixes.toString() + "\n ecpPrefixes:"
                        + matchPrefixList.toString() );

        // objects do not match delimiter are displayed in
        // contents:dir1?test1_18571.png, dir1?test3_18571
        List<String> actContentsList = new ArrayList<>();
        List<S3ObjectSummary> objects = result.getObjectSummaries();
        for ( S3ObjectSummary os : objects ) {
            String key = os.getKey();
            actContentsList.add( key );
        }

        // check the keyName
        Collections.sort( matchContentsList );
        Assert.assertEquals( actContentsList, matchContentsList );
    }
}
