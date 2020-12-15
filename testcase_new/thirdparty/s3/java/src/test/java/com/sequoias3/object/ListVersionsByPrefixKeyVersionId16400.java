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
 * @Description: seqDB-16400 :: 带prefix、keyMarker和versionIdMarker匹配查询对象版本列表
 * @author fanyu
 * @Date:2018年11月19日
 * @version:1.0
 */

public class ListVersionsByPrefixKeyVersionId16400 extends S3TestBase {
    private boolean runSuccess1 = false;
    private boolean runSuccess2 = false;
    private String bucketName = "bucket16400";
    private String[] objectNames = { "dir16400$dir16400A$dir16400AB",
            "dir16400$subdir16400A", "dirsub16400A", "dirsub16400B" };
    private List< String > objectNameList = new ArrayList<>();
    private AmazonS3 s3Client = null;
    private int versionNum = 500;

    @BeforeClass
    private void setUp() throws IOException {
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
        CommLib.setBucketVersioning( s3Client, bucketName,
                BucketVersioningConfiguration.ENABLED );
        for ( String objectName : objectNames ) {
            for ( int i = 0; i < versionNum; i++ ) {
                objectNameList.add( objectName );
                s3Client.putObject( bucketName, objectName,
                        "" + UUID.randomUUID() );
            }
        }
    }

    // 匹配对象可以一次返回（如小于默认一次返回数1000）
    @Test
    private void testRecordsLt1K() throws Exception {
        String prefix = "dirsub";
        // String prefix = "dir";
        int index = 3;
        String keyMarker = objectNames[ index ];
        String versionIdMarker = String.valueOf( versionNum );
        // list by prefix/keyMarker/versionIdMarker
        VersionListing vsList = s3Client.listVersions(
                new ListVersionsRequest().withBucketName( bucketName )
                        .withPrefix( prefix ).withKeyMarker( keyMarker )
                        .withVersionIdMarker( versionIdMarker ) );

        // expected results
        MultiValueMap< String, String > expMap = new LinkedMultiValueMap< String, String >();
        for ( int i = index; i < objectNames.length; i++ ) {
            for ( int j = versionNum - 1; j >= 0; j-- ) {
                expMap.add( objectNames[ i ], String.valueOf( j ) );
            }
        }
        // check
        Assert.assertEquals( vsList.isTruncated(), false,
                "vsList.isTruncated() must be false" );
        ObjectUtils.checkListVSResults( vsList, new ArrayList< String >(),
                expMap );
        runSuccess1 = true;
    }

    // 匹配对象分多次查询返回（如大于默认一次返回数1000）
    @Test
    private void testRecordsGt1K() throws Exception {
        String prefix = "dir";
        int index = 0;
        String keyMarker = objectNames[ index ];
        String versionIdMarker = String.valueOf( versionNum );
        VersionListing vsList = null;
        int count = 0;
        do {
            // list by prefix/keyMarker/versionIdMarker
            vsList = s3Client.listVersions(
                    new ListVersionsRequest().withBucketName( bucketName )
                            .withPrefix( prefix ).withKeyMarker( keyMarker )
                            .withVersionIdMarker( versionIdMarker ) );

            // check
            MultiValueMap< String, String > expMap = new LinkedMultiValueMap< String, String >();
            for ( int i = 0; i < vsList.getMaxKeys() / versionNum; i++ ) {
                for ( int j = versionNum - 1; j >= 0; j--, count++ ) {
                    expMap.add( objectNameList.get( count ),
                            String.valueOf( j ) );
                }
            }
            ObjectUtils.checkListVSResults( vsList, new ArrayList< String >(),
                    expMap );
            keyMarker = vsList.getNextKeyMarker();
            versionIdMarker = vsList.getNextVersionIdMarker();
        } while ( vsList.isTruncated() );
        runSuccess2 = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess1 && runSuccess2 ) {
                CommLib.clearBucket( s3Client, bucketName );
            }
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }
}
