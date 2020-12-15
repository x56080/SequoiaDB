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
 * @Description: seqDB-18158 :To get a listVersions within a bucket.specify
 *               keyMarker/delimiter/versionIdMarker.no matching keyMarker
 * @author wuyan
 * @Date:2019.4.25
 * @version:1.0
 */

public class ListVersions18158 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket18158";
    private String delimiter = "!";
    private AmazonS3 s3Client = null;
    private String[] keyNames = { "a!test0_18158", "atest1_18158",
            "atest2!18158.png", "test@!3_18158", "test@a4_18158",
            "test@!5_18158", "testa_test6_18158", "testa_x7!_18158",
            "y/test8!_18158" };

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
    private void testListVersions() {
        String keyMarker = "yyyy";
        String versionIdMarker = "1";
        VersionListing vsList = s3Client.listVersions(
                new ListVersionsRequest().withBucketName( bucketName )
                        .withDelimiter( delimiter ).withKeyMarker( keyMarker )
                        .withVersionIdMarker( versionIdMarker ) );

        List< String > matchPrefixList = new ArrayList<>();
        MultiValueMap< String, String > expVersions = new LinkedMultiValueMap< String, String >();

        Assert.assertEquals( vsList.isTruncated(), false, "keyMarker:"
                + keyMarker + "  list.isTruncated() is unexpected!" );
        ObjectUtils.checkListVSResults( vsList, matchPrefixList, expVersions );
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
