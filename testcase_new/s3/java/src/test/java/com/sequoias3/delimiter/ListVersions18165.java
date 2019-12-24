package com.sequoias3.delimiter;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.BucketVersioningConfiguration;
import com.amazonaws.services.s3.model.ListVersionsRequest;
import com.amazonaws.services.s3.model.S3VersionSummary;
import com.amazonaws.services.s3.model.VersionListing;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.DelimiterUtils;
import org.springframework.util.LinkedMultiValueMap;
import org.springframework.util.MultiValueMap;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Map;

/**
 * @Description: seqDB-18165 :To get a listVersions within a bucket.specify
 *               prefix and delimiter. the number of matching objects of the
 *               currentVersion less than 100,and matching objects of all
 *               versions greater than maxkeys.
 * @author wuyan
 * @Date:2019.4.30
 * @version:1.0
 */

public class ListVersions18165 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket18165";
    private String keyName = "object18165";
    private String prefix = "dir1/";
    private String delimiter = "%";
    private int maxKeys = 80;
    private List<String> matchPrefixList = new ArrayList<>();
    private MultiValueMap<String, String> expVersions = new LinkedMultiValueMap<String, String>();
    private AmazonS3 s3Client = null;
    private int versionNum = 5;
    private int objectsNum = 90;
    private int deleteTagObjectsNum = 5;

    @BeforeClass
    private void setUp() throws IOException {
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
        CommLib.setBucketVersioning( s3Client, bucketName,
                BucketVersioningConfiguration.ENABLED );
        putObjects();
        DelimiterUtils.putBucketDelimiter( bucketName, delimiter );
    }

    @Test
    private void testListVersions() throws Exception {
        // list versions by prefix/delimiter/maxKes, for first query
        VersionListing vsList = s3Client.listVersions(
                new ListVersionsRequest().withBucketName( bucketName )
                        .withDelimiter( delimiter ).withPrefix( prefix )
                        .withMaxResults( maxKeys ) );
        List<String> actCommonPrefixes = vsList.getCommonPrefixes();
        List<S3VersionSummary> vsSummaryList = vsList.getVersionSummaries();
        MultiValueMap<String, String> actVersionMap = new LinkedMultiValueMap<String, String>();
        for ( S3VersionSummary versionSummary : vsSummaryList ) {
            actVersionMap.add( versionSummary.getKey(),
                    versionSummary.getVersionId() );
        }

        // check matchNums for first query
        Assert.assertTrue( vsList.isTruncated(),
                "vsList.isTruncated() must be true" );
        int matchPrefixNums = 50;
        int matchVersionMapNums = 6;
        Assert.assertEquals( actCommonPrefixes.size(), matchPrefixNums );
        Assert.assertEquals( actVersionMap.size(), matchVersionMapNums );

        // second query
        String nextKeyMarker = vsList.getNextKeyMarker();
        String nextVersionIdMarker = vsList.getNextVersionIdMarker();
        VersionListing vsList1 = s3Client.listVersions(
                new ListVersionsRequest().withBucketName( bucketName )
                        .withKeyMarker( nextKeyMarker )
                        .withVersionIdMarker( nextVersionIdMarker )
                        .withDelimiter( delimiter ) );

        List<String> actCommonPrefixes1 = vsList1.getCommonPrefixes();
        List<S3VersionSummary> vsSummaryList1 = vsList1.getVersionSummaries();
        MultiValueMap<String, String> actVersionMap1 = new LinkedMultiValueMap<String, String>();
        for ( S3VersionSummary versionSummary : vsSummaryList1 ) {
            actVersionMap1.add( versionSummary.getKey(),
                    versionSummary.getVersionId() );
        }

        // check the matchPrefix And Versions Result
        Assert.assertFalse( vsList1.isTruncated(),
                "vsList1.isTruncated() must be false" );
        actCommonPrefixes1.addAll( actCommonPrefixes );
        MultiValueMap<String, String> expVersions = new LinkedMultiValueMap<String, String>();
        // actVersionMap1.addAll(actVersionMap);
        for ( Map.Entry<String, List<String>> entry : actVersionMap
                .entrySet() ) {
            List<String> versions = entry.getValue();
            for ( String version : versions ) {
                actVersionMap1.add( entry.getKey(), version );
            }
        }
        checkMatchResult( actCommonPrefixes1, actVersionMap1 );

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

    private void putObjects() {
        for ( int i = 0; i < objectsNum; i++ ) {
            if ( i % 10 == 0 ) {
                // keyName match prefix,no match delimiter
                String subKeyName = prefix + "_" + i + "_" + keyName;
                for ( int j = versionNum - 1; j >= 0; j-- ) {
                    s3Client.putObject( bucketName, subKeyName,
                            subKeyName + "_" + i + "_" + j );
                    expVersions.add( subKeyName, String.valueOf( j ) );
                }

            } else {
                // keyName match prefix and delimter
                String subKeyName =
                        prefix + "_" + i + delimiter + "_" + keyName;
                for ( int j = versionNum - 1; j >= 0; j-- ) {
                    s3Client.putObject( bucketName, subKeyName,
                            subKeyName + "_" + i + "_" + j );
                }
                matchPrefixList.add( prefix + "_" + i + delimiter );
            }
        }

        // put delete tag object
        for ( int i = 0; i < deleteTagObjectsNum; i++ ) {
            String subKeyName =
                    prefix + "_deleteTag_" + i + delimiter + "_" + keyName;
            s3Client.deleteObject( bucketName, subKeyName );
            matchPrefixList.add( prefix + "_deleteTag_" + i + delimiter );
        }
    }

    private void checkMatchResult( List<String> actCommonPrefixes,
            MultiValueMap<String, String> actVersionMap ) {
        Collections.sort( actCommonPrefixes );
        Collections.sort( matchPrefixList );
        Assert.assertEquals( actCommonPrefixes, matchPrefixList,
                "actCommonPrefixes = " + actCommonPrefixes.toString()
                        + ",expCommonPrefixes = " + matchPrefixList
                        .toString() );
        Assert.assertEquals( actVersionMap.size(), expVersions.size(),
                "actMap = " + actVersionMap.toString() + ",expMap = "
                        + expVersions.toString() );
        for ( Map.Entry<String, List<String>> entry : expVersions.entrySet() ) {
            Assert.assertEquals( actVersionMap.get( entry.getKey() ),
                    expVersions.get( entry.getKey() ),
                    "actMap = " + actVersionMap.toString() + ",expMap = "
                            + expVersions.toString() );
        }

    }
}
