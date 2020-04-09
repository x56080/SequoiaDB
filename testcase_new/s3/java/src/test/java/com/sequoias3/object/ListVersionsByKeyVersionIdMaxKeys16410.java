package com.sequoias3.object;

import java.io.IOException;
import java.util.ArrayList;
import java.util.UUID;

import org.springframework.util.LinkedMultiValueMap;
import org.springframework.util.MultiValueMap;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.BucketVersioningConfiguration;
import com.amazonaws.services.s3.model.ListVersionsRequest;
import com.amazonaws.services.s3.model.VersionListing;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.ObjectUtils;

/**
 * @Description: seqDB-16410 :: 带keyMarker、versionIdMarker和maxkeys查询对象版本列表
 * @author fanyu
 * @Date:2018年11月23日
 * @version:1.0
 */

public class ListVersionsByKeyVersionIdMaxKeys16410 extends S3TestBase {
    private boolean runSuccess1 = false;
    private boolean runSuccess2 = false;
    private boolean runSuccess3 = false;
    private boolean runSuccess4 = false;
    private String bucketName = "bucket16410";
    private String[] objectNames = { "123#16410", "234#16410", "345#16410",
            "456#16410", "567#16410" };
    private AmazonS3 s3Client = null;
    private int versionNum = 10;

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

    // keyMarker、versionIdMarker匹配从第一条记录开始，设置maxkeys小于对象数
    @Test
    private void testHead() throws Exception {
        String keyMarker = objectNames[ 0 ];
        String versionIdMarker = String.valueOf( versionNum );
        Integer maxResults = versionNum * ( objectNames.length - 1 );
        VersionListing vsList = s3Client.listVersions( new ListVersionsRequest()
                .withBucketName( bucketName ).withKeyMarker( keyMarker )
                .withVersionIdMarker( versionIdMarker )
                .withMaxResults( maxResults ) );

        // expected results
        MultiValueMap< String, String > expMap = new LinkedMultiValueMap< String, String >();
        for ( int i = 0; i < objectNames.length - 1; i++ ) {
            for ( int j = versionNum - 1; j >= 0; j-- ) {
                expMap.add( objectNames[ i ], String.valueOf( j ) );
            }
        }
        // check
        Assert.assertEquals( vsList.isTruncated(), true,
                "vsList.isTruncated() must be false" );
        ObjectUtils.checkListVSResults( vsList, new ArrayList< String >(),
                expMap );
        runSuccess1 = true;
    }

    // keyMarker、versionIdMarker匹配最后1条记录，设置maxkeys大于1
    @Test
    private void testTail1() throws Exception {
        String keyMarker = objectNames[ objectNames.length - 1 ];
        String versionIdMarker = "1";
        Integer maxResults = 2;
        VersionListing vsList = s3Client.listVersions( new ListVersionsRequest()
                .withBucketName( bucketName ).withKeyMarker( keyMarker )
                .withVersionIdMarker( versionIdMarker )
                .withMaxResults( maxResults ) );
        // expected results
        MultiValueMap< String, String > expMap = new LinkedMultiValueMap< String, String >();
        expMap.add( objectNames[ objectNames.length - 1 ],
                String.valueOf( 0 ) );
        // check
        Assert.assertEquals( vsList.isTruncated(), false,
                "vsList.isTruncated() must be false" );
        ObjectUtils.checkListVSResults( vsList, new ArrayList< String >(),
                expMap );
        runSuccess2 = true;
    }

    // keyMarker、versionIdMarker指定最后1条对象key，指定maxkeys为1
    @Test
    private void testTail2() throws Exception {
        String keyMarker = objectNames[ objectNames.length - 1 ];
        String versionIdMarker = "0";
        Integer maxResults = 1;
        VersionListing vsList = s3Client.listVersions( new ListVersionsRequest()
                .withBucketName( bucketName ).withKeyMarker( keyMarker )
                .withVersionIdMarker( versionIdMarker )
                .withMaxResults( maxResults ) );
        // check
        Assert.assertEquals( vsList.isTruncated(), false,
                "vsList.isTruncated() must be false" );
        ObjectUtils.checkListVSResults( vsList, new ArrayList< String >(),
                new LinkedMultiValueMap< String, String >() );
        runSuccess3 = true;
    }

    // keyMarker、versionIdMarker指定从第1条对象key开始，指定maxkeys为0
    @Test
    private void testMaxKeyZero() throws Exception {
        String keyMarker = objectNames[ 0 ];
        String versionIdMarker = "0";
        Integer maxResults = 0;
        VersionListing vsList = s3Client.listVersions( new ListVersionsRequest()
                .withBucketName( bucketName ).withKeyMarker( keyMarker )
                .withVersionIdMarker( versionIdMarker )
                .withMaxResults( maxResults ) );
        // check
        Assert.assertEquals( vsList.isTruncated(), false,
                "vsList.isTruncated() must be false" );
        ObjectUtils.checkListVSResults( vsList, new ArrayList< String >(),
                new LinkedMultiValueMap< String, String >() );
        runSuccess4 = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess1 && runSuccess2 && runSuccess3 && runSuccess4 ) {
                CommLib.clearBucket( s3Client, bucketName );
            }
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }
}
