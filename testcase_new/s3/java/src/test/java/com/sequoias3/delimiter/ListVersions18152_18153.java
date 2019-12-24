package com.sequoias3.delimiter;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.BucketVersioningConfiguration;
import com.amazonaws.services.s3.model.ListVersionsRequest;
import com.amazonaws.services.s3.model.VersionListing;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import org.springframework.util.LinkedMultiValueMap;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.IOException;
import java.util.ArrayList;

/**
 * @Description: seqDB-18152 :To get a listVersions within a bucket.specify
 *               prefix/keyMarker/delimiter/versionIdMarker. match
 *               prefix/delimiter/versionIdMarker, no match keyMarker.
 *               seqDB-18153 :no match versionIdMarker
 * @author wuyan
 * @Date 2019.4.24
 * @version:1.0
 */

public class ListVersions18152_18153 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket18152";
    private String[] keyNames = { "dir1/a/test1_18152.png",
            "dir1/dir2/a/test2_18152.png", "/a/test3_18152.png",
            "test4_18152.png" };
    private AmazonS3 s3Client = null;

    @BeforeClass
    private void setUp() throws IOException {
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
        CommLib.setBucketVersioning( s3Client, bucketName,
                BucketVersioningConfiguration.ENABLED );
        for ( String keyName : keyNames ) {
            s3Client.putObject( bucketName, keyName,
                    "oldVersionContent_" + keyName );
            s3Client.putObject( bucketName, keyName,
                    "newVersionContent_" + keyName );
        }
    }

    @Test
    private void test() throws Exception {
        // keyMarker does not exist
        String keyMarker = "test18152";
        String versionIdMarker = "1";
        String prefix = "dir";
        String delimiter = "/";
        // list by prefix/keyMarker/versionIdMarker/delimiter
        VersionListing vsList = s3Client.listVersions(
                new ListVersionsRequest().withBucketName( bucketName )
                        .withPrefix( prefix ).withKeyMarker( keyMarker )
                        .withVersionIdMarker( versionIdMarker )
                        .withDelimiter( delimiter ) );

        ObjectUtils.checkListVSResults( vsList, new ArrayList<String>(),
                new LinkedMultiValueMap<String, String>() );

        // versionIdMarker does not exist
        String keyMarker1 = "test18153";
        String versionIdMarker1 = "3";
        // list by prefix/keyMarker/versionIdMarker/delimiter
        VersionListing vsList1 = s3Client.listVersions(
                new ListVersionsRequest().withBucketName( bucketName )
                        .withPrefix( prefix ).withKeyMarker( keyMarker1 )
                        .withVersionIdMarker( versionIdMarker1 )
                        .withDelimiter( delimiter ) );

        ObjectUtils.checkListVSResults( vsList1, new ArrayList<String>(),
                new LinkedMultiValueMap<String, String>() );

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
