package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.BucketVersioningConfiguration;
import com.amazonaws.services.s3.model.ListVersionsRequest;
import com.amazonaws.services.s3.model.VersionListing;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
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
import java.util.UUID;

/**
 * @author fanyu
 * @Description: seqDB-16415 ::多次查询结果在commprefix中有相同记录
 * @Date:2018年11月24日
 * @version:1.0
 */

public class ListVersionsByPrefixDelimiterMaxkeys16415 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket16415";
    private String[] objectNames = { "/aa/bb/test1", "/aa/bb/test2",
            "/bb/cc/test1", "/bb/cc/test2", "/cc/dd/test1", "/cc/dd/test2" };
    private AmazonS3 s3Client = null;
    private int versionNum = 3;

    @BeforeClass
    private void setUp() throws IOException {
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
        CommLib.setBucketVersioning( s3Client, bucketName,
                BucketVersioningConfiguration.ENABLED );
        for ( String objectName : objectNames ) {
            for ( int j = 0; j < versionNum; j++ ) {
                s3Client.putObject( bucketName, objectName,
                        "" + UUID.randomUUID() );
            }
        }
    }

    @Test // SEQUOIADBMAINSTREAM-3987
    private void test1() throws Exception {
        String prefix = "/";
        String delimiter = "/";
        Integer maxResults = 1;

        VersionListing vsList = s3Client.listVersions(
                new ListVersionsRequest().withBucketName( bucketName )
                        .withPrefix( prefix ).withDelimiter( delimiter )
                        .withMaxResults( maxResults ) );
        // expected results
        List<String> expCommonPrefixes = new ArrayList<String>();
        expCommonPrefixes.add( "/aa/" );

        // chceck
        Assert.assertEquals( vsList.isTruncated(), true,
                "vsList.isTruncated() must be true" );
        ObjectUtils.checkListVSResults( vsList, expCommonPrefixes,
                new LinkedMultiValueMap<String, String>() );

        // test isTruncated
        String nextKeyMarker = vsList.getNextKeyMarker();
        String nextVersionIdMarker = vsList.getNextVersionIdMarker();
        Integer maxResults2 = 2;
        VersionListing vsList2 = s3Client.listVersions(
                new ListVersionsRequest().withBucketName( bucketName )
                        .withKeyMarker( nextKeyMarker )
                        .withVersionIdMarker( nextVersionIdMarker )
                        .withPrefix( prefix ).withDelimiter( delimiter )
                        .withMaxResults( maxResults2 ) );

        // expected results
        List<String> expCommonPrefixes2 = new ArrayList<String>();
        expCommonPrefixes2.add( "/bb/" );
        expCommonPrefixes2.add( "/cc/" );
        Assert.assertEquals( vsList2.isTruncated(), false,
                "vsList3.isTruncated() must be false" );
        ObjectUtils.checkListVSResults( vsList2, expCommonPrefixes2,
                new LinkedMultiValueMap<String, String>() );

        // add new object
        String newObjectName = "/dd";
        for ( int j = 0; j < versionNum; j++ ) {
            s3Client.putObject( bucketName, newObjectName,
                    "" + UUID.randomUUID() );
        }

        // test isTruncated
        Integer maxResults3 = 100;
        VersionListing vsList3 = s3Client.listVersions(
                new ListVersionsRequest().withBucketName( bucketName )
                        .withKeyMarker( nextKeyMarker )
                        .withVersionIdMarker( nextVersionIdMarker )
                        .withPrefix( prefix ).withDelimiter( delimiter )
                        .withMaxResults( maxResults3 ) );

        // expected results
        List<String> expCommonPrefixes3 = new ArrayList<String>();
        expCommonPrefixes3.add( "/bb/" );
        expCommonPrefixes3.add( "/cc/" );
        MultiValueMap<String, String> expMap = new LinkedMultiValueMap<String, String>();
        for ( int k = versionNum - 1; k >= 0; k-- ) {
            expMap.add( newObjectName, String.valueOf( k ) );
        }
        Assert.assertEquals( vsList3.isTruncated(), false,
                "vsList3.isTruncated() must be false" );
        ObjectUtils.checkListVSResults( vsList3, expCommonPrefixes3, expMap );
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
}
