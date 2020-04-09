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
 * @Description: seqDB-18155 :To get a listVersions within a bucket.specify
 *               prefix/keyMarker/delimiter/versionIdMarker.list query multiple.
 * @author wuyan
 * @Date:2019.4.25
 * @version:1.0
 */

public class ListVersions18155 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket18155";
    private String delimiter = "?";
    private AmazonS3 s3Client = null;
    private int versionNum = 3;
    private int maxKeys = 5;
    private String prefix = "dir1/";
    private String[] keyNames = { "a/test0_18155", "a/test1_18155",
            "dir1/atest2_18155.png", "dir1/dir2/dir3/test?3_18155",
            "dir1/test?4_18155", "dir1/test5_18155", "dir1/dir2/xx?test6_18155",
            "fdir1/test7?_18155", "dir1/test?8_18155",
            "dir1/dir2/aa?dd/test9_18155", "dir1/test10_18155",
            "testdir1?11.txt_18155" };

    @BeforeClass
    private void setUp() throws IOException {
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
        CommLib.setBucketVersioning( s3Client, bucketName,
                BucketVersioningConfiguration.ENABLED );

        for ( int i = 0; i < keyNames.length; i++ ) {
            s3Client.putObject( bucketName, keyNames[ i ],
                    "testContest_" + keyNames[ i ] + "_version1" );
            s3Client.putObject( bucketName, keyNames[ i ],
                    "testContest_" + keyNames[ i ] + "_version1" );
            s3Client.putObject( bucketName, keyNames[ i ],
                    "testContest_" + keyNames[ i ] + "_version3" );
        }

        DelimiterUtils.putBucketDelimiter( bucketName, delimiter );
    }

    @Test
    private void testListVersions() throws Exception {
        // first query
        List< String > matchPrefixList = new ArrayList<>();
        matchPrefixList.add( "dir1/dir2/dir3/test?" );
        matchPrefixList.add( "dir1/dir2/aa?" );
        MultiValueMap< String, String > expVersions = new LinkedMultiValueMap< String, String >();
        // expContent:"dir1/atest2_18155.png",the versionId is:2,1,0
        for ( int i = versionNum - 1; i >= 0; i-- ) {
            expVersions.add( keyNames[ 2 ], String.valueOf( i ) );
        }
        VersionListing vsList = listVersionWithContentsAndCheckResult( null,
                matchPrefixList, expVersions, true );

        // second query
        MultiValueMap< String, String > expVersions1 = new LinkedMultiValueMap< String, String >();
        // expContent:"dir1/test10_18155",the versionId
        // is:2,1,0;"dir1/test5_18155",the versionId is:2
        for ( int i = versionNum - 1; i >= 0; i-- ) {
            expVersions1.add( keyNames[ 10 ], String.valueOf( i ) );
        }
        expVersions1.add( keyNames[ 5 ], String.valueOf( 2 ) );
        List< String > matchPrefixList1 = new ArrayList<>();
        matchPrefixList1.add( "dir1/dir2/xx?" );
        VersionListing vsList1 = listVersionWithContentsAndCheckResult( vsList,
                matchPrefixList1, expVersions1, true );

        // third query
        MultiValueMap< String, String > expVersions2 = new LinkedMultiValueMap< String, String >();
        // expContent:"dir1/test5_18155",the versionId is:1,0
        expVersions2.add( keyNames[ 5 ], String.valueOf( 1 ) );
        expVersions2.add( keyNames[ 5 ], String.valueOf( 0 ) );
        List< String > matchPrefixList2 = new ArrayList<>();
        matchPrefixList2.add( "dir1/test?" );

        listVersionWithContentsAndCheckResult( vsList1, matchPrefixList2,
                expVersions2, false );

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                CommLib.clearBucket( s3Client, bucketName );
            }
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }

    private VersionListing listVersionWithContentsAndCheckResult(
            VersionListing vsList, List< String > expCommonPrefixes,
            MultiValueMap< String, String > expVersions, boolean isTruncate ) {
        String keyMarker = keyNames[ 1 ];
        String versionIdMarker = "2";
        if ( vsList != null ) {
            keyMarker = vsList.getNextKeyMarker();
            versionIdMarker = vsList.getNextVersionIdMarker();
        }

        VersionListing vsList1 = s3Client
                .listVersions( new ListVersionsRequest()
                        .withBucketName( bucketName ).withDelimiter( delimiter )
                        .withPrefix( prefix ).withKeyMarker( keyMarker )
                        .withVersionIdMarker( versionIdMarker )
                        .withMaxResults( maxKeys ) );

        Assert.assertEquals( vsList1.isTruncated(), isTruncate, "keyMarker:"
                + keyMarker + "  list.isTruncated() is unexpected!" );
        ObjectUtils.checkListVSResults( vsList1, expCommonPrefixes,
                expVersions );

        return vsList1;

    }
}
