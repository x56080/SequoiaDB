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
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

/**
 * @Description: seqDB-16403 ::
 *               带prefix、keyMarker、versionIdMarker和delimiter匹配查询对象版本列表
 * @author fanyu
 * @Date:2018年11月20日
 * @version:1.0
 */

public class ListVersionsByPrefixDelimiterIDKey16403 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket16403";
    // please sort in an ascending order by objectName
    private String[] objectNames = { "air116403", "dir2/16403A.png",
            "dir3/16403.xml", "test16403.doc" };
    private AmazonS3 s3Client = null;
    private int fileSize = 3;
    private int versionNum = 5;
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
    }

    @Test
    private void test() throws Exception {
        String prefix = "dir";
        String delimiter = "/";
        String keyMarker = objectNames[ 0 ];

        // expected results
        List<String> expCommPrefixes = ObjectUtils
                .getCommPrefixes( objectNames, prefix, delimiter );

        // list versions by prefix/delimiter/currentversionId/key
        String versionIdMarker1 = "4";
        VersionListing vsList = s3Client.listVersions(
                new ListVersionsRequest().withBucketName( bucketName )
                        .withDelimiter( delimiter ).withPrefix( prefix )
                        .withKeyMarker( keyMarker )
                        .withVersionIdMarker( versionIdMarker1 ) );

        // check
        Assert.assertEquals( vsList.isTruncated(), false,
                "vsList.isTruncated() must be false" );
        ObjectUtils.checkListVSResults( vsList, expCommPrefixes,
                new LinkedMultiValueMap<String, String>() );

        // list versions by prefix/delimiter/histversionId/key
        String versionIdMarker2 = "3";
        VersionListing vsList1 = s3Client.listVersions(
                new ListVersionsRequest().withBucketName( bucketName )
                        .withDelimiter( delimiter ).withPrefix( prefix )
                        .withKeyMarker( keyMarker )
                        .withVersionIdMarker( versionIdMarker2 ) );

        // check
        Assert.assertEquals( vsList1.isTruncated(), false,
                "vsList1.isTruncated() must be false" );
        ObjectUtils.checkListVSResults( vsList1, expCommPrefixes,
                new LinkedMultiValueMap<String, String>() );
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
