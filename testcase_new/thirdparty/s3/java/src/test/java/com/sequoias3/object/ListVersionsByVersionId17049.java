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
 * @Description: seqDB-17049:指定versionIdMarker != null查询对象版本列表，且版本列表中有versionId
 *               == null的记录
 * @author fanyu
 * @Date:2019年01月04日
 * @version:1.0
 */

public class ListVersionsByVersionId17049 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket17049";
    private String[] objectNames = { "17049:012", "17049:345", "17049:678",
            "17049:9AB", "17049:CDE" };
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
            s3Client.putObject( bucketName, objectName,
                    "" + UUID.randomUUID() );
        }
        CommLib.setBucketVersioning( s3Client, bucketName,
                BucketVersioningConfiguration.SUSPENDED );
        for ( String objectName : objectNames ) {
            s3Client.putObject( bucketName, objectName,
                    "" + UUID.randomUUID() );
        }
        CommLib.setBucketVersioning( s3Client, bucketName,
                BucketVersioningConfiguration.ENABLED );
        for ( String objectName : objectNames ) {
            s3Client.putObject( bucketName, objectName,
                    "" + UUID.randomUUID() );
        }
    }

    // null的内部versionId小于versionIdMarker记录
    @Test
    private void test() throws Exception {
        int index = 0;
        String keyMarker = objectNames[ index ];
        int versionIdMarker = versionNum;
        Integer maxResults = 7;

        VersionListing vsList = s3Client.listVersions( new ListVersionsRequest()
                .withBucketName( bucketName ).withKeyMarker( keyMarker )
                .withVersionIdMarker( String.valueOf( versionIdMarker ) )
                .withMaxResults( maxResults ) );

        // expected results
        MultiValueMap< String, String > expMap = new LinkedMultiValueMap< String, String >();
        for ( int i = index; i < maxResults / versionNum; i++ ) {
            for ( int j = versionNum - 1; j >= 0; j-- ) {
                if ( j != 1 ) {
                    expMap.add( objectNames[ i ], String.valueOf( j ) );
                } else {
                    expMap.add( objectNames[ i ], "null" );
                }
            }
        }
        expMap.add( objectNames[ 2 ], "2" );

        Assert.assertTrue( vsList.isTruncated(),
                "vsList.isTruncated() must be true" );
        ObjectUtils.checkListVSResults( vsList, new ArrayList< String >(),
                expMap );

        // null的内部versionId大于versionIdMarker记录
        Integer maxResults1 = 7;
        String nextKeyMarker = vsList.getNextKeyMarker();
        String nextVersionIdMarker = "1";

        VersionListing vsList1 = s3Client.listVersions(
                new ListVersionsRequest().withBucketName( bucketName )
                        .withKeyMarker( nextKeyMarker )
                        .withVersionIdMarker(
                                String.valueOf( nextVersionIdMarker ) )
                        .withMaxResults( maxResults1 ) );

        // expected results
        MultiValueMap< String, String > expMap1 = new LinkedMultiValueMap< String, String >();
        expMap1.add( objectNames[ 2 ], String.valueOf( 0 ) );
        for ( int i = 3; i < objectNames.length; i++ ) {
            for ( int j = versionNum - 1; j >= 0; j-- ) {
                if ( j != 1 ) {
                    expMap1.add( objectNames[ i ], String.valueOf( j ) );
                } else {
                    expMap1.add( objectNames[ i ], "null" );
                }
            }
        }
        Assert.assertFalse( vsList1.isTruncated(),
                "vsList.isTruncated() must be false" );
        ObjectUtils.checkListVSResults( vsList1, new ArrayList< String >(),
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
