package com.sequoias3.delimiter;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.BucketVersioningConfiguration;
import com.amazonaws.services.s3.model.ListVersionsRequest;
import com.amazonaws.services.s3.model.VersionListing;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.DelimiterUtils;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import org.springframework.util.LinkedMultiValueMap;
import org.springframework.util.MultiValueMap;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

/**
 * @Description: seqDB-18160 :To get a listVersions multiple within a bucket.
 *               specify that the nextContinuationToken match object is deleted.
 * @author wuyan
 * @Date:2019.4.30
 * @version:1.0
 */

public class ListVersions18160 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket18160";
    private String delimiter = "t";
    private AmazonS3 s3Client = null;
    private int maxKeys = 5;
    private String[] keyNames = { "a/test0_18160", "a/test1_18160",
            "dir1/atest2_18160.png", "dir1/dir2/dir3/test/3_18160",
            "dir1/test?4_18160", "dir1/test5_18160", "dir1/xx?test6_18160",
            "fdir1/dir/7_18160", "hdir1/test/8_18160",
            "testdir1?11.txt_18160" };

    @BeforeClass
    private void setUp() throws IOException {
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
        CommLib.setBucketVersioning( s3Client, bucketName,
                BucketVersioningConfiguration.ENABLED );

        for ( int i = 0; i < keyNames.length; i++ ) {
            s3Client.putObject( bucketName, keyNames[ i ],
                    "testContest_" + keyNames[ i ] );
        }

        DelimiterUtils.putBucketDelimiter( bucketName, delimiter );
    }

    @Test
    private void testListVersions() throws Exception {
        // first query
        VersionListing vsList = s3Client.listVersions(
                new ListVersionsRequest().withBucketName( bucketName )
                        .withDelimiter( delimiter ).withMaxResults( maxKeys ) );

        // check query result
        List<String> matchPrefixList = new ArrayList<>();
        matchPrefixList.add( "a/t" );
        matchPrefixList.add( "dir1/at" );
        matchPrefixList.add( "dir1/dir2/dir3/t" );
        matchPrefixList.add( "dir1/xx?t" );
        matchPrefixList.add( "dir1/t" );
        MultiValueMap<String, String> expVersions = new LinkedMultiValueMap<String, String>();
        Assert.assertTrue( vsList.isTruncated(),
                "vsList.isTruncated() must be true" );
        ObjectUtils.checkListVSResults( vsList, matchPrefixList, expVersions );

        // delete the match object of the
        // nextKeyMarker(eg:keyNames[6])
        String nextKeyMarker = vsList.getNextKeyMarker();
        String defalueVersionId = "0";
        s3Client.deleteVersion( bucketName, keyNames[ 6 ], defalueVersionId );

        VersionListing vsList1 = s3Client.listVersions(
                new ListVersionsRequest().withBucketName( bucketName )
                        .withKeyMarker( nextKeyMarker )
                        .withDelimiter( delimiter ) );

        // expected results
        List<String> matchPrefixList1 = new ArrayList<>();
        matchPrefixList1.add( "hdir1/t" );
        matchPrefixList1.add( "t" );
        MultiValueMap<String, String> expVersions1 = new LinkedMultiValueMap<String, String>();
        expVersions1.add( keyNames[ 7 ], defalueVersionId );
        Assert.assertFalse( vsList1.isTruncated(),
                "vsList1.isTruncated() must be false" );
        ObjectUtils
                .checkListVSResults( vsList1, matchPrefixList1, expVersions1 );

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                for ( int i = 0; i < keyNames.length; i++ ) {
                    s3Client.deleteVersion( bucketName, keyNames[ i ], "0" );
                }
                s3Client.deleteBucket( bucketName );
            }
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }
}
