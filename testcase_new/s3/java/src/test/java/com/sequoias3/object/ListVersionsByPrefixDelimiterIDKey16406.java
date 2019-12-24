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
import java.util.Collections;
import java.util.List;

/**
 * @Description: seqDB-16406:带prefix、keyMarker、
 *               versionIdMarker和delimiter匹配查询对象版本列表（多次查询）
 * @author fanyu
 * @Date:2018年11月20日
 * @version:1.0
 */

public class ListVersionsByPrefixDelimiterIDKey16406 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket16406";
    private String objectNameBase = "dir";
    private List<String> objectNames = new ArrayList<String>();
    private int objectNum = 1000;
    private AmazonS3 s3Client = null;
    private int fileSize = 1;
    private int versionNum = 3;
    private File localPath = null;
    private List<String> filePathList = new ArrayList<String>();

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator + TestTools
                .getClassName() );
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        for ( int i = 0; i < versionNum; i++ ) {
            String filePath =
                    localPath + File.separator + "localFile_" + ( fileSize + i )
                            + ".txt";
            TestTools.LocalFile.createFile( filePath, fileSize + i );
            filePathList.add( filePath );
        }
        for ( int i = 0; i < objectNum / 2; i++ ) {
            objectNames.add( objectNameBase + "/16406-" + i );
        }
        for ( int i = objectNum / 2; i < objectNum; i++ ) {
            objectNames.add( objectNameBase + ":16406-" + i );
        }
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
        CommLib.setBucketVersioning( s3Client, bucketName,
                BucketVersioningConfiguration.ENABLED );
        for ( int i = 0; i < objectNum; i++ ) {
            for ( int j = 0; j < versionNum; j++ ) {
                s3Client.putObject(
                        new PutObjectRequest( bucketName, objectNames.get( i ),
                                new File( filePathList.get( j ) ) ) );
            }
        }
    }

    @Test
    private void test1() throws Exception {
        String prefix = "dir";
        String delimiter = "/";
        String keyMarker = objectNames.get( 0 );
        String versionIdMarker = String.valueOf( versionNum );

        // list versions by prefix/delimiter/versionIdMarker/keyMarker
        VersionListing vsList = s3Client.listVersions(
                new ListVersionsRequest().withBucketName( bucketName )
                        .withDelimiter( delimiter ).withPrefix( prefix )
                        .withKeyMarker( keyMarker )
                        .withVersionIdMarker( versionIdMarker ) );

        // expected results
        Collections.sort( objectNames );
        MultiValueMap<String, String> expMap = new LinkedMultiValueMap<String, String>();
        for ( int i = objectNum / 2; i <= objectNum / 2 + 1000 / 3; i++ ) {
            if ( i == objectNum / 2 + 1000 / 3 ) {
                expMap.add( objectNames.get( i ),
                        String.valueOf( versionNum - 1 ) );
            } else {
                for ( int j = versionNum - 1; j >= 0; j-- ) {
                    expMap.add( objectNames.get( i ), String.valueOf( j ) );
                }
            }
        }
        // check
        Assert.assertEquals( vsList.isTruncated(), true,
                "vsList.isTruncated() must be true" );
        ObjectUtils
                .checkListVSResults( vsList, new ArrayList<String>(), expMap );

        // list versions by prefix/delimiter/versionIdMarker/keyMarker
        String keyMarker1 = vsList.getNextKeyMarker();
        String versionIdMarker1 = vsList.getNextVersionIdMarker();
        VersionListing vsList1 = s3Client.listVersions(
                new ListVersionsRequest().withBucketName( bucketName )
                        .withDelimiter( delimiter ).withPrefix( prefix )
                        .withKeyMarker( keyMarker1 )
                        .withVersionIdMarker( versionIdMarker1 ) );

        // expected results
        MultiValueMap<String, String> expMap1 = new LinkedMultiValueMap<String, String>();
        for ( int i = objectNum / 2 + 1000 / 3; i < objectNum; i++ ) {
            if ( i == objectNum / 2 + 1000 / 3 ) {
                for ( int j = versionNum - 2; j >= 0; j-- ) {
                    expMap1.add( objectNames.get( i ), String.valueOf( j ) );
                }
            } else {
                for ( int j = versionNum - 1; j >= 0; j-- ) {
                    expMap1.add( objectNames.get( i ), String.valueOf( j ) );
                }
            }
        }
        // check
        Assert.assertEquals( vsList1.isTruncated(), false,
                "vsList1.isTruncated() must be false" );
        ObjectUtils.checkListVSResults( vsList1, new ArrayList<String>(),
                expMap1 );
        runSuccess = true;
    }

    @Test
    private void test2() throws Exception {
        String prefix = "dir";
        String delimiter = "/";
        String keyMarker = "air";
        String versionIdMarker = String.valueOf( versionNum );

        // list versions by prefix/delimiter/versionIdMarker/keyMarker
        VersionListing vsList = s3Client.listVersions(
                new ListVersionsRequest().withBucketName( bucketName )
                        .withDelimiter( delimiter ).withPrefix( prefix )
                        .withKeyMarker( keyMarker )
                        .withVersionIdMarker( versionIdMarker ) );

        // expected results
        Collections.sort( objectNames );
        List<String> expCommprefixes = new ArrayList<String>();
        expCommprefixes.add( "dir/" );

        MultiValueMap<String, String> expMap = new LinkedMultiValueMap<String, String>();
        for ( int i = objectNum / 2; i <= objectNum / 2 + 1000 / 3 - 1; i++ ) {
            for ( int j = versionNum - 1; j >= 0; j-- ) {
                expMap.add( objectNames.get( i ), String.valueOf( j ) );
            }
        }
        // check
        Assert.assertEquals( vsList.isTruncated(), true,
                "vsList.isTruncated() must be true" );
        ObjectUtils.checkListVSResults( vsList, expCommprefixes, expMap );

        // list versions by prefix/delimiter/versionIdMarker/keyMarker
        String keyMarker1 = vsList.getNextKeyMarker();
        String versionIdMarker1 = vsList.getNextVersionIdMarker();
        VersionListing vsList1 = s3Client.listVersions(
                new ListVersionsRequest().withBucketName( bucketName )
                        .withDelimiter( delimiter ).withPrefix( prefix )
                        .withKeyMarker( keyMarker1 )
                        .withVersionIdMarker( versionIdMarker1 ) );

        // expected results
        MultiValueMap<String, String> expMap1 = new LinkedMultiValueMap<String, String>();
        for ( int i = objectNum / 2 + 1000 / 3; i < objectNum; i++ ) {
            for ( int j = versionNum - 1; j >= 0; j-- ) {
                expMap1.add( objectNames.get( i ), String.valueOf( j ) );
            }
        }
        // check
        Assert.assertEquals( vsList1.isTruncated(), false,
                "vsList1.isTruncated() must be false" );
        ObjectUtils.checkListVSResults( vsList1, new ArrayList<String>(),
                expMap1 );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
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
