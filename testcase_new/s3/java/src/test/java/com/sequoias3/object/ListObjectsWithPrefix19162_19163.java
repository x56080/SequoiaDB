package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.BucketVersioningConfiguration;
import com.amazonaws.services.s3.model.ListObjectsV2Request;
import com.amazonaws.services.s3.model.ListObjectsV2Result;
import com.amazonaws.services.s3.model.ListVersionsRequest;
import com.amazonaws.services.s3.model.S3ObjectSummary;
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
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

/**
 * @Description seqDB-19162:指定prefix为空串查询对象元数据列表 *
 *              seqDB-19163:指定prefix为空串查询对象版本列表
 * @author wangkexin
 * @Date 2019.08.15
 * @version 1.00
 */
public class ListObjectsWithPrefix19162_19163 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket19162";
    private String[] objectNames = { "dir1/19162", "dir1/dir2/19162",
            "dir1/dir3/19162", "19162" };
    private List<String> objectNameList = new ArrayList<>();
    private String prefix = "";
    private int versionNum = 10;
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * 2;
    private File localPath = null;
    private String filePath = null;

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator + TestTools
                .getClassName() );
        filePath =
                localPath + File.separator + "localFile_" + fileSize + ".txt";

        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath, fileSize );
        s3Client = CommLib.buildS3Client();

        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
        CommLib.setBucketVersioning( s3Client, bucketName,
                BucketVersioningConfiguration.ENABLED );
        for ( String objectName : objectNames ) {
            for ( int i = 0; i < versionNum; i++ ) {
                objectNameList.add( objectName );
                s3Client.putObject( bucketName, objectName,
                        new File( filePath ) );
            }
        }
    }

    @Test
    public void testListObjects() throws Exception {
        listObjectsAndCheckResult();
        listVersionsAndCheckResult();
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
            s3Client.shutdown();
        }
    }

    private void listObjectsAndCheckResult() {
        ListObjectsV2Request request = new ListObjectsV2Request()
                .withBucketName( bucketName ).withEncodingType( "url" );
        request.withPrefix( prefix );
        ListObjectsV2Result result = s3Client.listObjectsV2( request );
        List<String> commonPrefixes = result.getCommonPrefixes();
        Assert.assertEquals( commonPrefixes.size(), 0,
                "commonPrefixes : " + commonPrefixes.toString() );

        List<String> actContentList = new ArrayList<>();
        List<S3ObjectSummary> objects = result.getObjectSummaries();
        for ( S3ObjectSummary os : objects ) {
            actContentList.add( os.getKey() );
        }
        // check the keyName
        List<String> expContentList = Arrays.asList( objectNames );
        Collections.sort( expContentList );
        Assert.assertEquals( actContentList, expContentList );
    }

    private void listVersionsAndCheckResult() {
        ListVersionsRequest request = new ListVersionsRequest()
                .withBucketName( bucketName );
        request.withPrefix( prefix );
        VersionListing vsList = s3Client.listVersions( request );
        MultiValueMap<String, String> expMap = new LinkedMultiValueMap<String, String>();
        for ( int i = 0; i < objectNames.length; i++ ) {
            for ( int j = versionNum - 1; j >= 0; j-- ) {
                expMap.add( objectNames[ i ], String.valueOf( j ) );
            }
        }
        // check
        Assert.assertEquals( vsList.isTruncated(), false,
                "vsList.isTruncated() must be false" );
        ObjectUtils
                .checkListVSResults( vsList, new ArrayList<String>(), expMap );
    }
}
