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
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

/**
 * @Description: seqDB-16405
 *               ::带prefix、keyMarker、versionIdMarker和delimiter查询对象版本列表，不匹配prefix
 * @author fanyu
 * @Date:2018年11月20日
 * @version:1.0
 */

public class ListVersionsByPrefixDelimiterIDKey16405 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket16405";
    // please sort in an ascending order by objectName
    private String[] objectNames = { "air116405", "b16405/A.png",
            "test16405.doc" };
    private AmazonS3 s3Client = null;
    private int fileSize = 3;
    private int versionNum = 3;
    private File localPath = null;
    private List< String > filePathList = new ArrayList< String >();

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator
                + TestTools.getClassName() );
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        for ( int i = 0; i < versionNum; i++ ) {
            String filePath = localPath + File.separator + "localFile_"
                    + ( fileSize + i ) + ".txt";
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
                s3Client.putObject( new PutObjectRequest( bucketName,
                        objectName, new File( filePathList.get( i ) ) ) );
            }
        }
    }

    @Test
    private void test() throws Exception {
        String prefix = "dir";
        String delimiter = "/";
        String keyMarker = objectNames[ 0 ];
        String versionIdMarker = "3";

        // list versions by prefix/delimiter/currentversionId/key
        VersionListing vsList = s3Client.listVersions( new ListVersionsRequest()
                .withBucketName( bucketName ).withDelimiter( delimiter )
                .withPrefix( prefix ).withKeyMarker( keyMarker )
                .withVersionIdMarker( versionIdMarker ) );
        // check results
        ObjectUtils.checkListVSResults( vsList, new ArrayList< String >(),
                new LinkedMultiValueMap< String, String >() );
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
