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
import java.util.Collections;
import java.util.List;
import java.util.UUID;

/**
 * @Description:seqDB-16399 :: 带prefix、keyMarker和versionIdMarker查询对象版本列表，
 *                          不匹配versionIdMarker
 * @author fanyu
 * @Date:2018年11月19日
 * @version:1.0
 */

public class ListVersionsByPrefixKeyVersionId16399 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket16399";
    private String[] objectNames = { "dir16399%subdir16399A",
            "dir16399%dir16399A%dir16399AB", "dir16399A", "dir16399B" };
    private List< String > sortObjectNames = new ArrayList< String >();
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
            for ( int i = 0; i < versionNum; i++ ) {
                s3Client.putObject( bucketName, objectName,
                        "" + UUID.randomUUID() );
            }
            sortObjectNames.add( objectName );
        }
        Collections.sort( sortObjectNames );
    }

    @Test
    private void test() throws Exception {
        String prefix = "dir";
        int index = 0;
        String keyMarker = sortObjectNames.get( index );
        String versionIdMarker = String.valueOf( versionNum );
        // list by prefix/keyMarker/versionIdMarker
        VersionListing vsList = s3Client.listVersions(
                new ListVersionsRequest().withBucketName( bucketName )
                        .withPrefix( prefix ).withKeyMarker( keyMarker )
                        .withVersionIdMarker( versionIdMarker ) );
        // expected results
        MultiValueMap< String, String > expMap = new LinkedMultiValueMap< String, String >();
        for ( String objectName : sortObjectNames ) {
            for ( int i = versionNum - 1; i >= 0; i-- ) {
                expMap.add( objectName, String.valueOf( i ) );
            }
        }
        // check
        Assert.assertEquals( vsList.isTruncated(), false,
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
