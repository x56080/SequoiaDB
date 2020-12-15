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
 * @Description: seqDB-16407 :: 带delimiter和maxkeys查询对象版本列表
 * @author fanyu
 * @Date:2018年11月23日
 * @version:1.0
 */

public class ListVersionsByDelimiterMaxKeys16407 extends S3TestBase {
    private boolean runSuccess1 = false;
    private boolean runSuccess2 = false;
    private String bucketName = "bucket16407";
    private String[] objectNames = { "air/16407", "dir/16407", "fire16407",
            "test16407" };
    private AmazonS3 s3Client = null;
    private int versionNum = 4;

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

    // 指定maxkeys一次返回所有匹配条件的对象
    @Test
    private void testGtMaxKeys() throws Exception {
        String delimiter = "/";
        // maxResults > versionNum*objectNames.length
        VersionListing vsList = s3Client.listVersions( new ListVersionsRequest()
                .withBucketName( bucketName ).withDelimiter( delimiter )
                .withMaxResults( versionNum * objectNames.length + 1 ) );

        // expected results
        List< String > expCommonPrefixes = new ArrayList< String >();
        expCommonPrefixes.add( "air/" );
        expCommonPrefixes.add( "dir/" );

        MultiValueMap< String, String > expMap = new LinkedMultiValueMap< String, String >();
        for ( int i = 2; i < objectNames.length; i++ ) {
            for ( int j = versionNum - 1; j >= 0; j-- ) {
                expMap.add( objectNames[ i ], String.valueOf( j ) );
            }
        }

        if ( !vsList.isTruncated() ) {
            ObjectUtils.checkListVSResults( vsList, expCommonPrefixes, expMap );
        } else {
            Assert.fail( "vsList.isTruncated() must be false" );
        }
        runSuccess1 = true;
    }

    // 指定maxkeys多次返回所有匹配条件的对象
    @Test
    private void testLtMaxKeys() throws Exception {
        String delimiter = "/";
        // maxResults < versionNum*objectNames.length
        VersionListing vsList = s3Client.listVersions(
                new ListVersionsRequest().withBucketName( bucketName )
                        .withDelimiter( delimiter ).withMaxResults( 1 ) );
        List< String > expCommonPrefixes1 = new ArrayList< String >();
        expCommonPrefixes1.add( "air/" );
        if ( vsList.isTruncated() ) {
            ObjectUtils.checkListVSResults( vsList, expCommonPrefixes1,
                    new LinkedMultiValueMap< String, String >() );
        } else {
            Assert.fail( "vsList.isTruncated() must be true" );
        }

        // maxResults > versionNum*objectNames.length
        String nextKeyMarker = vsList.getNextKeyMarker();
        String nestVersionIdMarker = vsList.getNextVersionIdMarker();
        VersionListing vsList1 = s3Client.listVersions(
                new ListVersionsRequest().withBucketName( bucketName )
                        .withKeyMarker( nextKeyMarker )
                        .withVersionIdMarker( nestVersionIdMarker )
                        .withDelimiter( delimiter )
                        .withMaxResults( versionNum * objectNames.length ) );
        // expected results
        List< String > expCommonPrefixes2 = new ArrayList< String >();
        expCommonPrefixes2.add( "dir/" );
        MultiValueMap< String, String > expMap = new LinkedMultiValueMap< String, String >();
        for ( int i = 2; i < objectNames.length; i++ ) {
            for ( int j = versionNum - 1; j >= 0; j-- ) {
                expMap.add( objectNames[ i ], String.valueOf( j ) );
            }
        }
        if ( !vsList1.isTruncated() ) {
            ObjectUtils.checkListVSResults( vsList1, expCommonPrefixes2,
                    expMap );
        } else {
            Assert.fail( "vsList.isTruncated() must be false" );
        }
        runSuccess2 = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess1 && runSuccess2 ) {
                CommLib.clearBucket( s3Client, bucketName );
            }
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }
}
