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
 * @Description: seqDB-16394 :: 指定不同格式分割符查询
 * @author fanyu
 * @Date:2018年11月26日
 * @version:1.0
 */

public class ListVersionsBySpecialDelimiter16394 extends S3TestBase {
    private boolean runSuccess1 = false;
    private boolean runSuccess2 = false;
    private boolean runSuccess3 = false;
    private boolean runSuccess4 = false;
    private String bucketName = "bucket16394";
    private String[] objectNames = { "!-_.*',()", "&@: $=+?", "^`><{}[]#%\"~|",
            "a1b2C3D4", "thisis\010atest", "abc" };
    private AmazonS3 s3Client = null;

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
    }

    // a、字母数字字符[0-9a-zA-Z]
    @Test
    private void testNormal() throws Exception {
        String delimiter = objectNames[ 5 ];
        VersionListing vsList = s3Client.listVersions( new ListVersionsRequest()
                .withBucketName( bucketName ).withDelimiter( delimiter ) );

        // expected results
        List< String > expCommonPrefixes = new ArrayList< String >();
        expCommonPrefixes.add( objectNames[ 5 ] );
        MultiValueMap< String, String > expMap = new LinkedMultiValueMap< String, String >();
        expMap.add( objectNames[ 0 ], "0" );
        expMap.add( objectNames[ 1 ], "0" );
        expMap.add( objectNames[ 2 ], "0" );
        expMap.add( objectNames[ 3 ], "0" );
        expMap.add( objectNames[ 4 ], "0" );

        // check
        Assert.assertEquals( vsList.isTruncated(), false,
                "vsList.isTruncated() must be false" );
        ObjectUtils.checkListVSResults( vsList, expCommonPrefixes, expMap );
        runSuccess1 = true;
    }

    // b、""
    @Test
    private void testEmptyStr() throws Exception {
        VersionListing vsList = s3Client.listVersions( new ListVersionsRequest()
                .withBucketName( bucketName ).withDelimiter( "" ) );

        // expected results
        MultiValueMap< String, String > expMap = new LinkedMultiValueMap< String, String >();
        expMap.add( objectNames[ 0 ], "0" );
        expMap.add( objectNames[ 1 ], "0" );
        expMap.add( objectNames[ 2 ], "0" );
        expMap.add( objectNames[ 3 ], "0" );
        expMap.add( objectNames[ 4 ], "0" );
        expMap.add( objectNames[ 5 ], "0" );

        // check
        Assert.assertEquals( vsList.isTruncated(), false,
                "vsList.isTruncated() must be false" );
        ObjectUtils.checkListVSResults( vsList, new ArrayList< String >(),
                expMap );
        runSuccess1 = true;
    }

    // 特殊字符：“!”，“-”，“_”，“.”，“*”，“'”，“(”，“)”
    @Test
    private void testSpecial() throws Exception {
        String str = objectNames[ 0 ];
        char[] delimiters = str.toCharArray();
        MultiValueMap< String, String > expMap = new LinkedMultiValueMap< String, String >();
        expMap.add( objectNames[ 1 ], "0" );
        expMap.add( objectNames[ 2 ], "0" );
        expMap.add( objectNames[ 3 ], "0" );
        expMap.add( objectNames[ 4 ], "0" );
        expMap.add( objectNames[ 5 ], "0" );
        for ( int i = 0; i < delimiters.length; i++ ) {
            VersionListing vsList = s3Client
                    .listVersions( new ListVersionsRequest()
                            .withBucketName( bucketName ).withDelimiter(
                                    String.valueOf( delimiters[ i ] ) ) );
            List< String > expCommonPrefixes = new ArrayList< String >();
            expCommonPrefixes.add( str.substring( 0, i + 1 ) );
            Assert.assertEquals( vsList.isTruncated(), false,
                    "vsList.isTruncated() must be false" );
            ObjectUtils.checkListVSResults( vsList, expCommonPrefixes, expMap );
        }
        runSuccess2 = true;
    }

    // 要特殊处理的字符：&@:,$=+?;
    @Test
    private void testSpecial1() throws Exception {
        String str = objectNames[ 1 ];
        char[] delimiters = str.toCharArray();
        MultiValueMap< String, String > expMap = new LinkedMultiValueMap< String, String >();
        expMap.add( objectNames[ 0 ], "0" );
        expMap.add( objectNames[ 2 ], "0" );
        expMap.add( objectNames[ 3 ], "0" );
        expMap.add( objectNames[ 4 ], "0" );
        expMap.add( objectNames[ 5 ], "0" );
        for ( int i = 0; i < delimiters.length; i++ ) {
            VersionListing vsList = s3Client
                    .listVersions( new ListVersionsRequest()
                            .withBucketName( bucketName ).withDelimiter(
                                    String.valueOf( delimiters[ i ] ) ) );
            List< String > expCommonPrefixes = new ArrayList< String >();
            expCommonPrefixes.add( str.substring( 0, i + 1 ) );
            Assert.assertEquals( vsList.isTruncated(), false,
                    "vsList.isTruncated() must be false" );
            ObjectUtils.checkListVSResults( vsList, expCommonPrefixes, expMap );
        }
        runSuccess3 = true;
    }

    // "^`><{}[]#%\"~|"
    @Test
    private void testSpecial3() throws Exception {
        String str = objectNames[ 2 ];
        char[] delimiters = str.toCharArray();
        MultiValueMap< String, String > expMap = new LinkedMultiValueMap< String, String >();
        expMap.add( objectNames[ 0 ], "0" );
        expMap.add( objectNames[ 1 ], "0" );
        expMap.add( objectNames[ 3 ], "0" );
        expMap.add( objectNames[ 4 ], "0" );
        expMap.add( objectNames[ 5 ], "0" );
        for ( int i = 0; i < delimiters.length; i++ ) {
            VersionListing vsList = s3Client
                    .listVersions( new ListVersionsRequest()
                            .withBucketName( bucketName ).withDelimiter(
                                    String.valueOf( delimiters[ i ] ) ) );
            List< String > expCommonPrefixes = new ArrayList< String >();
            expCommonPrefixes.add( str.substring( 0, i + 1 ) );
            Assert.assertEquals( vsList.isTruncated(), false,
                    "vsList.isTruncated() must be false" );
            ObjectUtils.checkListVSResults( vsList, expCommonPrefixes, expMap );
        }
        runSuccess4 = true;
    }

    // ""this is\012atest!!!"
    @Test
    private void testSpecial5() throws Exception {
        String str = objectNames[ 4 ];
        String delimiter = "\010";
        MultiValueMap< String, String > expMap = new LinkedMultiValueMap< String, String >();
        expMap.add( objectNames[ 0 ], "0" );
        expMap.add( objectNames[ 1 ], "0" );
        expMap.add( objectNames[ 2 ], "0" );
        expMap.add( objectNames[ 3 ], "0" );
        expMap.add( objectNames[ 5 ], "0" );

        VersionListing vsList = s3Client.listVersions( new ListVersionsRequest()
                .withBucketName( bucketName ).withDelimiter( delimiter ) );
        List< String > expCommonPrefixes = new ArrayList< String >();
        expCommonPrefixes.add( "thisis\010" );
        Assert.assertEquals( vsList.isTruncated(), false,
                "vsList.isTruncated() must be false" );
        ObjectUtils.checkListVSResults( vsList, expCommonPrefixes, expMap );
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
