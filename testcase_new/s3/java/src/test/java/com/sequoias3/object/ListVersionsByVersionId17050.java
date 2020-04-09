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
 * @Description: seqDB-17050:指定versionIdmarker == null查询对象版本列表，且版本列表中有versionId
 *               == null的记录
 * @author fanyu
 * @Date:2019年01月04日
 * @version:1.0
 */

public class ListVersionsByVersionId17050 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket17050";
    private String[] objectNames = { "17050%%123", "17050%%345", "17050%%567",
            "17050%%9AB", "17050%%CDE" };
    private AmazonS3 s3Client = null;
    private int versionNum = 3;

    @BeforeClass
    private void setUp() throws IOException {
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
        for ( String objectName : objectNames ) {
            s3Client.putObject( bucketName, objectName,
                    "" + UUID.randomUUID() );
        }
        CommLib.setBucketVersioning( s3Client, bucketName,
                BucketVersioningConfiguration.ENABLED );
        for ( String objectName : objectNames ) {
            for ( int i = 0; i < versionNum - 1; i++ ) {
                s3Client.putObject( bucketName, objectName,
                        "" + UUID.randomUUID() );
            }
        }
    }

    @Test
    private void test() throws Exception {
        int index = 0;
        String keyMarker = objectNames[ index ];
        String versionIdMarker = "null";

        VersionListing vsList = s3Client.listVersions( new ListVersionsRequest()
                .withBucketName( bucketName ).withKeyMarker( keyMarker )
                .withVersionIdMarker( versionIdMarker ) );

        // expected results
        MultiValueMap< String, String > expMap = new LinkedMultiValueMap< String, String >();
        for ( int i = index + 1; i < objectNames.length; i++ ) {
            for ( int j = versionNum - 1; j >= 0; j-- ) {
                if ( j != 0 ) {
                    expMap.add( objectNames[ i ], String.valueOf( j ) );
                } else {
                    expMap.add( objectNames[ i ], "null" );
                }
            }
        }
        Assert.assertFalse( vsList.isTruncated(),
                "vsList.isTruncated() must be false" );
        ObjectUtils.checkListVSResults( vsList, new ArrayList< String >(),
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
