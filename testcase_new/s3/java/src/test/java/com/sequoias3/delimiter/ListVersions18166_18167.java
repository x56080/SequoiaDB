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
 * @Description: seqDB-18166/18167 :To get a listVersions within a
 *               bucket.specify prefix and delimiter. the number of matching
 *               objects in the currentVersion greater than 100,and matching
 *               objects of all versions less than or equals maxkeys
 * @author wuyan
 * @Date:2019.5.5
 * @version:1.0
 */

public class ListVersions18166_18167 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket18166";
    private String keyName = "object18166";
    private String prefix = "dir1/";
    private String delimiter = "test";
    private List<String> matchPrefixList = new ArrayList<>();
    private MultiValueMap<String, String> expVersions = new LinkedMultiValueMap<String, String>();
    private AmazonS3 s3Client = null;
    private int versionNum = 3;
    private int objectsNum = 110;
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
        // testcase 18166: matching objects of all versions less than maxkeys
        int maxKeys1 = 200;
        listVersionAndCheckResult( maxKeys1 );

        // testcase 18167: matching objects of all versions equals maxkeys
        int maxKeys2 = 137;
        listVersionAndCheckResult( maxKeys2 );

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

    private void listVersionAndCheckResult( int maxKeys ) {
        // list versions by prefix/delimiter
        VersionListing vsList = s3Client.listVersions(
                new ListVersionsRequest().withBucketName( bucketName )
                        .withDelimiter( delimiter ).withPrefix( prefix )
                        .withMaxResults( maxKeys ) );

        // check
        Assert.assertEquals( vsList.isTruncated(), false,
                "vsList.isTruncated() must be false" );
        ObjectUtils.checkListVSResults( vsList, matchPrefixList, expVersions );
    }
}
