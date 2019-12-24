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
import java.util.UUID;

/**
 * @Description: seqDB-16414 :: 指定nextVersionIdMarker匹配记录被删除，查询版本列表信息
 * @author fanyu
 * @Date:2018年11月23日
 * @version:1.0
 */

public class ListVersionsByNextKeyMaxKey16414 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket16414";
    private String[] objectNames = { "16414%123", "16414%456", "16414%789",
            "16414%ABC", "16414%DEF" };
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

    @Test // bug:3986
    private void test() throws Exception {
        int index = 0;
        String keyMarker = objectNames[ index ];
        int versionIdMarker = versionNum;
        Integer maxResults = 7;

        VersionListing vsList = s3Client.listVersions(
                new ListVersionsRequest().withBucketName( bucketName )
                        .withKeyMarker( keyMarker ).withVersionIdMarker(
                        String.valueOf( versionIdMarker ) )
                        .withMaxResults( maxResults ) );

        // expected results
        MultiValueMap<String, String> expMap = new LinkedMultiValueMap<String, String>();
        for ( int i = index; i < maxResults / versionNum; i++ ) {
            for ( int j = versionNum - 1; j >= 0; j-- ) {
                expMap.add( objectNames[ i ], String.valueOf( j ) );
            }
        }
        expMap.add( objectNames[ 2 ], "2" );

        Assert.assertTrue( vsList.isTruncated(),
                "vsList.isTruncated() must be true" );
        ObjectUtils
                .checkListVSResults( vsList, new ArrayList<String>(), expMap );

        String nextKeyMarker = vsList.getNextKeyMarker();
        String nextVersionIdMarker = String.valueOf( 1 );
        s3Client.deleteVersion( bucketName, nextKeyMarker,
                nextVersionIdMarker );

        VersionListing vsList1 = s3Client.listVersions(
                new ListVersionsRequest().withBucketName( bucketName )
                        .withKeyMarker( nextKeyMarker ).withVersionIdMarker(
                        String.valueOf( nextVersionIdMarker ) )
                        .withMaxResults( maxResults ) );

        // expected results
        MultiValueMap<String, String> expMap1 = new LinkedMultiValueMap<String, String>();
        expMap1.add( objectNames[ 2 ], "0" );
        for ( int i = 3; i < objectNames.length; i++ ) {
            for ( int j = versionNum - 1; j >= 0; j-- ) {
                expMap1.add( objectNames[ i ], String.valueOf( j ) );
            }
        }
        Assert.assertFalse( vsList1.isTruncated(),
                "vsList.isTruncated() must be false" );
        ObjectUtils.checkListVSResults( vsList1, new ArrayList<String>(),
                expMap1 );
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
