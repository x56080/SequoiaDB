package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.BucketVersioningConfiguration;
import com.amazonaws.services.s3.model.ListVersionsRequest;
import com.amazonaws.services.s3.model.PutObjectRequest;
import com.amazonaws.services.s3.model.S3VersionSummary;
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
import java.util.Map;

/**
 * test content: 带prefix和delimiter查询对象版本列表，存在不匹配delimiter且最新记录为deletemarker的对象
 * testlink-case: seqDB-18960
 *
 * @author wangkexin
 * @Date 2019.7.31
 * @version 1.00
 */
public class ListVersionsByPrefixDelimiter18960 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket18960";
    private String[] objectNames = { "dir18960/test1/1.txt",
            "dir18960/test2/2.txt", "dir18960/test3/3.txt", "dir18960/test4",
            "dir18960/test5", "18960test6" };
    private AmazonS3 s3Client = null;
    private int fileSize = 3;
    private int versionNum = 2;
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
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
        CommLib.setBucketVersioning( s3Client, bucketName,
                BucketVersioningConfiguration.ENABLED );
        for ( String objectName : objectNames ) {
            for ( int i = 0; i < versionNum; i++ ) {
                s3Client.putObject(
                        new PutObjectRequest( bucketName, objectName,
                                new File( filePathList.get( i ) ) ) );
            }
        }

        // delete object "dir18960/test5"
        s3Client.deleteObject( bucketName, objectNames[ 4 ] );
    }

    @Test
    private void test() throws Exception {
        String prefix = "dir18960/";
        String delimiter = "/";
        VersionListing vsList = s3Client.listVersions(
                new ListVersionsRequest().withBucketName( bucketName )
                        .withPrefix( prefix ).withDelimiter( delimiter ) );
        // expected results
        List<String> expCommPrefixes = ObjectUtils
                .getCommPrefixes( objectNames, prefix, delimiter );
        List<String> versionKeys = ObjectUtils
                .getKeys( objectNames, prefix, delimiter );
        MultiValueMap<String, String> expMap = new LinkedMultiValueMap<String, String>();
        for ( String key : versionKeys ) {
            for ( int i = versionNum - 1; i >= 0; i-- ) {
                expMap.add( key, String.valueOf( i ) );
                expMap.add( key, "false" );
            }
            if ( key.equals( objectNames[ 4 ] ) ) {
                expMap.add( key, String.valueOf( versionNum ) );
                expMap.add( key, "true" );
            }
        }
        // check
        Assert.assertEquals( vsList.isTruncated(), false,
                "vsList.isTruncated() must be false" );
        checkListVSResults( vsList, expCommPrefixes, expMap );
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

    private void checkListVSResults( VersionListing vsList,
            List<String> expCommonPrefixes,
            MultiValueMap<String, String> expMap ) {
        Collections.sort( expCommonPrefixes );
        List<String> actCommonPrefixes = vsList.getCommonPrefixes();
        Assert.assertEquals( actCommonPrefixes, expCommonPrefixes,
                "actCommonPrefixes = " + actCommonPrefixes.toString()
                        + ",expCommonPrefixes = " + expCommonPrefixes
                        .toString() );
        List<S3VersionSummary> vsSummaryList = vsList.getVersionSummaries();
        MultiValueMap<String, String> actMap = new LinkedMultiValueMap<String, String>();
        for ( S3VersionSummary versionSummary : vsSummaryList ) {
            actMap.add( versionSummary.getKey(),
                    versionSummary.getVersionId() );
            actMap.add( versionSummary.getKey(),
                    String.valueOf( versionSummary.isDeleteMarker() ) );
        }
        Assert.assertEquals( actMap.size(), expMap.size(),
                "actMap = " + actMap.toString() + ",expMap = " + expMap
                        .toString() );
        for ( Map.Entry<String, List<String>> entry : expMap.entrySet() ) {
            Assert.assertEquals( actMap.get( entry.getKey() ),
                    expMap.get( entry.getKey() ),
                    "actMap = " + actMap.toString() + ",expMap = " + expMap
                            .toString() );
        }
    }
}
