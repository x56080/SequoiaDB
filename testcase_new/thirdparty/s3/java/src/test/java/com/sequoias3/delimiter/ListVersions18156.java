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
 * @Description: seqDB-18156 :To get a listVersions within a bucket.specify
 *               keyMarker/delimiter/versionIdMarker/maxkeys. test the
 *               following: test a: keyMarker is first object, the maxkeys <
 *               matching records; test b: keyMarker is last object, the maxkeys
 *               > 1; test c: keyMarker is last object, the maxkeys = 1.
 * @author wuyan
 * @Date:2019.4.25
 * @version:1.0
 */

public class ListVersions18156 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket18156";
    private String delimiter = "?";
    private AmazonS3 s3Client = null;
    private int versionNum = 3;
    private String[] keyNames = { "a?test0_18156", "atest1_18156",
            "atest2?18156.png", "test?3_18156", "test?4_18156",
            "testa_test5_18156", "testa_x6?_18156", "y/test7?_18156" };

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
        // test a: keyMarker is first object, the maxkeys < matching records
        String keyMarker = "a";
        String versionIdMarker = "2";
        int maxKeys = 6;
        testScenarioA( keyMarker, versionIdMarker, maxKeys );

        // test b: keyMarker is last object, the maxkeys > 1
        String keyMarkerB = keyNames[ 6 ];
        String versionIdMarkerB = "0";
        int maxKeysB = 6;
        testScenarioB( keyMarkerB, versionIdMarkerB, maxKeysB );

        // test c: keyMarker is last object, the maxkeys = 1
        String keyMarkerC = keyNames[ 7 ];
        String versionIdMarkerC = "1";
        int maxKeysC = 1;
        testScenarioC( keyMarkerC, versionIdMarkerC, maxKeysC );

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
            VersionListing vsList, String keyMarker, String versionIdMarker,
            int maxKeys, List< String > expCommonPrefixes,
            MultiValueMap< String, String > expVersions, boolean isTruncate ) {
        if ( vsList != null ) {
            keyMarker = vsList.getNextKeyMarker();
            versionIdMarker = vsList.getNextVersionIdMarker();
        }

        VersionListing vsList1 = s3Client.listVersions(
                new ListVersionsRequest().withBucketName( bucketName )
                        .withDelimiter( delimiter ).withKeyMarker( keyMarker )
                        .withVersionIdMarker( versionIdMarker )
                        .withMaxResults( maxKeys ) );

        Assert.assertEquals( vsList1.isTruncated(), isTruncate, "keyMarker:"
                + keyMarker + "  list.isTruncated() is unexpected!" );
        ObjectUtils.checkListVSResults( vsList1, expCommonPrefixes,
                expVersions );

        return vsList1;
    }

    private void testScenarioA( String keyMarker, String versionIdMarker,
            int maxKeys ) {
        List< String > matchPrefixList = new ArrayList<>();
        matchPrefixList.add( "a?" );
        matchPrefixList.add( "atest2?" );
        matchPrefixList.add( "test?" );
        MultiValueMap< String, String > expVersions = new LinkedMultiValueMap< String, String >();
        // expContent:"atest1_18155",the versionId is:2,1,0
        for ( int i = versionNum - 1; i >= 0; i-- ) {
            expVersions.add( keyNames[ 1 ], String.valueOf( i ) );
        }
        VersionListing vsList = listVersionWithContentsAndCheckResult( null,
                keyMarker, versionIdMarker, maxKeys, matchPrefixList,
                expVersions, true );

        // second query
        MultiValueMap< String, String > expVersions1 = new LinkedMultiValueMap< String, String >();
        // expContent:""testa_test5_18155"",the versionId is:2,1,0;
        for ( int i = versionNum - 1; i >= 0; i-- ) {
            expVersions1.add( keyNames[ 5 ], String.valueOf( i ) );
        }

        List< String > matchPrefixList1 = new ArrayList<>();
        matchPrefixList1.add( "testa_x6?" );
        matchPrefixList1.add( "y/test7?" );
        listVersionWithContentsAndCheckResult( vsList, "", "", maxKeys,
                matchPrefixList1, expVersions1, false );
    }

    private void testScenarioB( String keyMarker, String versionIdMarker,
            int maxKeys ) {
        List< String > matchPrefixList = new ArrayList<>();
        matchPrefixList.add( "y/test7?" );

        // expContent is null
        MultiValueMap< String, String > expVersions = new LinkedMultiValueMap< String, String >();

        listVersionWithContentsAndCheckResult( null, keyMarker, versionIdMarker,
                maxKeys, matchPrefixList, expVersions, false );
    }

    private void testScenarioC( String keyMarker, String versionIdMarker,
            int maxKeys ) {
        List< String > matchPrefixList = new ArrayList<>();

        // expContent is null
        MultiValueMap< String, String > expVersions = new LinkedMultiValueMap< String, String >();

        listVersionWithContentsAndCheckResult( null, keyMarker, versionIdMarker,
                maxKeys, matchPrefixList, expVersions, false );
    }
}
