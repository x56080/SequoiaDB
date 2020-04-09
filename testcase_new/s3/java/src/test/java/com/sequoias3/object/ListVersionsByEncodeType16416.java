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
import java.net.URLEncoder;
import java.util.ArrayList;
import java.util.List;
import java.util.UUID;

/**
 * @Description: seqDB-16416 :: 指定encoding-type查询对象版本列表
 * @author fanyu
 * @Date:2018年11月26日
 * @version:1.0
 */

public class ListVersionsByEncodeType16416 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket16416";
    private String[] objectNames = { "BEL", "16416!(16416 16416.txt).txt!",
            "16416!-/_!", "16416!.|*'!", "16416!#1", "16416!#2" };
    private AmazonS3 s3Client = null;
    private int versionNum = 2;

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

    @Test
    private void test() throws Exception {
        String prefix = "16416!";
        String delimiter = "!";
        String encodingType = "url";
        VersionListing vsList = s3Client.listVersions( new ListVersionsRequest()
                .withBucketName( bucketName ).withPrefix( prefix )
                .withDelimiter( delimiter ).withEncodingType( encodingType ) );

        // expected results
        List< String > expCommonPrefixes = ObjectUtils
                .getCommPrefixes( objectNames, prefix, delimiter );
        List< String > expCommonPrefixesByEncode = new ArrayList< String >();
        for ( String expCommonPrefix : expCommonPrefixes ) {
            expCommonPrefixesByEncode
                    .add( URLEncoder.encode( expCommonPrefix, "utf-8" ) );
        }
        MultiValueMap< String, String > expMap = new LinkedMultiValueMap< String, String >();
        for ( int i = 4; i < objectNames.length; i++ ) {
            for ( int j = versionNum - 1; j >= 0; j-- ) {
                expMap.add( URLEncoder.encode( objectNames[ i ], "utf-8" ),
                        String.valueOf( j ) );
            }
        }
        // check
        Assert.assertEquals( vsList.isTruncated(), false );
        ObjectUtils.checkListVSResults( vsList, expCommonPrefixesByEncode,
                expMap );
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
