package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.BucketVersioningConfiguration;
import com.amazonaws.services.s3.model.ListVersionsRequest;
import com.amazonaws.services.s3.model.PutObjectResult;
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
 * @Description: seqDB-16412 :: 带keyMarker、versionIdMarker查询对象版本列表，不匹配keyMarker
 * @author fanyu
 * @Date:2018年11月23日
 * @version:1.0
 */

public class ListVersionsByKeyVersionId16412 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket16412";
    private String[] objectNames = { "aaa%16412", "bbb%16412", "ccc%16412" };
    private List<PutObjectResult> objectList = new ArrayList<PutObjectResult>();
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
                PutObjectResult object = s3Client
                        .putObject( bucketName, objectName,
                                "" + UUID.randomUUID() );
                objectList.add( object );
            }
        }
    }

    @Test
    private void test() throws Exception {
        // keyMarker >= maxKeyMarker
        String keyMarker = objectNames[ objectNames.length - 1 ];
        int versionIdMarker = 0;
        VersionListing vsList = s3Client.listVersions(
                new ListVersionsRequest().withBucketName( bucketName )
                        .withKeyMarker( keyMarker ).withVersionIdMarker(
                        String.valueOf( versionIdMarker ) ) );
        // check
        Assert.assertEquals( vsList.isTruncated(), false,
                "vsList.isTruncated() must be false" );
        ObjectUtils.checkListVSResults( vsList, new ArrayList<String>(),
                new LinkedMultiValueMap<String, String>() );

        // keyMarker < maxKeyMarker
        String keyMarker1 = objectNames[ objectNames.length - 2 ];
        int versionIdMarker1 = 0;
        VersionListing vsList1 = s3Client.listVersions(
                new ListVersionsRequest().withBucketName( bucketName )
                        .withKeyMarker( keyMarker1 ).withVersionIdMarker(
                        String.valueOf( versionIdMarker1 ) ) );
        // expected results
        MultiValueMap<String, String> expMap = new LinkedMultiValueMap<String, String>();
        for ( int i = versionNum - 1; i >= 0; i-- ) {
            expMap.add( objectNames[ objectNames.length - 1 ],
                    String.valueOf( i ) );
        }
        // check
        Assert.assertEquals( vsList1.isTruncated(), false,
                "vsList.isTruncated() must be false" );
        ObjectUtils
                .checkListVSResults( vsList1, new ArrayList<String>(), expMap );
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
