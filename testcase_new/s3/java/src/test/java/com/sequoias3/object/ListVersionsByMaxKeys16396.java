package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.BucketVersioningConfiguration;
import com.amazonaws.services.s3.model.ListVersionsRequest;
import com.amazonaws.services.s3.model.PutObjectRequest;
import com.amazonaws.services.s3.model.VersionListing;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import org.springframework.util.LinkedMultiValueMap;
import org.springframework.util.MultiValueMap;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;

/**
 * @Description: seqDB-16396 ::带maxkeys查询对象版本列表
 * @author fanyu
 * @Date:2018年11月15日
 * @version:1.0
 */

public class ListVersionsByMaxKeys16396 extends S3TestBase {
    private boolean runSuccess1 = false;
    private boolean runSuccess2 = false;
    private boolean runSuccess3 = false;
    private boolean runSuccess4 = false;
    private String bucketName = "bucket16396";
    private String[] objectNames = { "dir16396%dir16396A%dir16396AB",
            "dir16396%subdir16396A", "dir16396A", "dir16396B" };
    private AmazonS3 s3Client = null;
    private int fileSize = 3;
    private File localPath = null;
    private String filePath = null;

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator
                + TestTools.getClassName() );
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        filePath = localPath + File.separator + "localFile_" + fileSize
                + ".txt";
        TestTools.LocalFile.createFile( filePath, fileSize );
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
        CommLib.setBucketVersioning( s3Client, bucketName,
                BucketVersioningConfiguration.ENABLED );
        for ( String objectName : objectNames ) {
            s3Client.putObject( new PutObjectRequest( bucketName, objectName,
                    new File( filePath ) ) );
        }
    }

    @Test
    private void testMaxKeysLtRecords() throws Exception {
        int maxKeys = 1;
        VersionListing vsList = s3Client.listVersions( new ListVersionsRequest()
                .withBucketName( bucketName ).withMaxResults( maxKeys ) );
        // expected results
        MultiValueMap< String, String > expMap = new LinkedMultiValueMap< String, String >();
        expMap.add( objectNames[ 0 ], String.valueOf( 0 ) );
        // check
        Assert.assertEquals( vsList.isTruncated(), true,
                "vsList.isTruncated() must be true" );
        ObjectUtils.checkListVSResults( vsList, new ArrayList< String >(),
                expMap );
        runSuccess1 = true;
    }

    @Test
    private void testMaxKeysGtRecords() throws Exception {
        int maxKeys = 5;
        VersionListing vsList = s3Client.listVersions( new ListVersionsRequest()
                .withBucketName( bucketName ).withMaxResults( maxKeys ) );
        // expected results
        MultiValueMap< String, String > expMap = new LinkedMultiValueMap< String, String >();
        for ( int i = 0; i < objectNames.length; i++ ) {
            expMap.add( objectNames[ i ], String.valueOf( 0 ) );
        }
        // check
        Assert.assertEquals( vsList.isTruncated(), false,
                "vsList.isTruncated() must be false" );
        ObjectUtils.checkListVSResults( vsList, new ArrayList< String >(),
                expMap );
        runSuccess2 = true;
    }

    @Test
    private void testMaxKeysEqRecords() throws Exception {
        int maxKeys = 4;
        VersionListing vsList = s3Client.listVersions( new ListVersionsRequest()
                .withBucketName( bucketName ).withMaxResults( maxKeys ) );
        // expected results
        MultiValueMap< String, String > expMap = new LinkedMultiValueMap< String, String >();
        for ( int i = 0; i < objectNames.length; i++ ) {
            expMap.add( objectNames[ i ], String.valueOf( 0 ) );
        }
        // check
        Assert.assertEquals( vsList.isTruncated(), false,
                "vsList.isTruncated() must be false" );
        ObjectUtils.checkListVSResults( vsList, new ArrayList< String >(),
                expMap );
        runSuccess3 = true;
    }

    @Test
    private void testMaxKeysEqMiddle() throws Exception {
        int maxKeys = 3;
        VersionListing vsList = s3Client.listVersions( new ListVersionsRequest()
                .withBucketName( bucketName ).withMaxResults( maxKeys ) );
        // expected results
        MultiValueMap< String, String > expMap = new LinkedMultiValueMap< String, String >();
        for ( int i = 0; i < maxKeys; i++ ) {
            expMap.add( objectNames[ i ], String.valueOf( 0 ) );
        }
        // check
        Assert.assertEquals( vsList.isTruncated(), true,
                "vsList.isTruncated() must be false" );
        ObjectUtils.checkListVSResults( vsList, new ArrayList< String >(),
                expMap );
        runSuccess4 = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess1 && runSuccess2 && runSuccess3 && runSuccess4 ) {
                CommLib.clearBucket( s3Client, bucketName );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }
}
