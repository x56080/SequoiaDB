package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.ListObjectsRequest;
import com.amazonaws.services.s3.model.ListObjectsV2Request;
import com.amazonaws.services.s3.model.ListObjectsV2Result;
import com.amazonaws.services.s3.model.ObjectListing;
import com.amazonaws.services.s3.model.S3ObjectSummary;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
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
 * @Description seqDB-16422: listObjectV2 with mathcing delimiter. seqDB-18562:
 *              listObjectV1 with mathcing delimiter.
 * @author wuyan
 * @Date 2018.11.19
 * @version 1.00
 */
public class ListObjectsWithDelimiter16422_18562 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket16422";
    private String key = "%aa%bb%object16422";
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * 2;
    private int matchObjectNums = 40;
    private File localPath = null;
    private String filePath = null;
    private String delimiter = "%aa%";

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator
                + TestTools.getClassName() );
        filePath = localPath + File.separator + "localFile_" + fileSize
                + ".txt";

        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath, fileSize );
        s3Client = CommLib.buildS3Client();

        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
    }

    @Test
    public void testListObjects() throws Exception {
        List< String > keyList = putObjects();
        listObjectsAndCheckResult( keyList );
        listObjectV1AndCheckResult( keyList );
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

    private void listObjectsAndCheckResult( List< String > keyList )
            throws IOException {
        List< String > queryKeyList = new ArrayList<>();
        ListObjectsV2Request request = new ListObjectsV2Request()
                .withBucketName( bucketName ).withEncodingType( "url" );
        request.withDelimiter( delimiter );
        ListObjectsV2Result result = s3Client.listObjectsV2( request );
        List< String > commonPrefixes = result.getCommonPrefixes();
        // matching delimiter displays only 1 record
        Assert.assertEquals( commonPrefixes.size(), 1 );
        Assert.assertEquals( commonPrefixes.get( 0 ), delimiter );

        // objects do not match delimiter are displayed in contents,num is 10
        List< S3ObjectSummary > objects = result.getObjectSummaries();
        int contentsNums = 10;
        Assert.assertEquals( objects.size(), contentsNums );
        for ( S3ObjectSummary os : objects ) {
            String key = os.getKey();
            String etag = os.getETag();
            long size = os.getSize();
            queryKeyList.add( key );
            Assert.assertEquals( etag, TestTools.getMD5( filePath ) );
            Assert.assertEquals( size, fileSize );
        }

        // check the keyName
        Assert.assertEquals( queryKeyList, keyList );
    }

    private void listObjectV1AndCheckResult( List< String > keyList )
            throws IOException {
        List< String > queryKeyList = new ArrayList<>();
        ListObjectsRequest request = new ListObjectsRequest()
                .withBucketName( bucketName );
        request.withDelimiter( delimiter );
        ObjectListing result = s3Client.listObjects( request );
        Assert.assertEquals( result.getDelimiter(), delimiter );

        List< String > commonPrefixes = result.getCommonPrefixes();
        // matching delimiter displays only 1 record
        Assert.assertEquals( commonPrefixes.size(), 1 );
        Assert.assertEquals( commonPrefixes.get( 0 ), delimiter );

        // objects do not match delimiter are displayed in contents,num is 10
        List< S3ObjectSummary > objects = result.getObjectSummaries();
        int contentsNums = 10;
        Assert.assertEquals( objects.size(), contentsNums );
        for ( S3ObjectSummary os : objects ) {
            String key = os.getKey();
            String etag = os.getETag();
            long size = os.getSize();
            queryKeyList.add( key );
            Assert.assertEquals( etag, TestTools.getMD5( filePath ) );
            Assert.assertEquals( size, fileSize );
        }

        // check the keyName
        Assert.assertEquals( queryKeyList, keyList );
    }

    private List< String > putObjects() {
        List< String > noMatchKeyList = new ArrayList<>();
        int objectNums = 50;
        String keyName;
        for ( int i = 0; i < objectNums; i++ ) {
            if ( i < matchObjectNums ) {
                keyName = key + "_" + i + TestTools.getRandomString( i );
            } else {
                keyName = "object16422_" + i;
                noMatchKeyList.add( keyName );
            }
            s3Client.putObject( bucketName, keyName, new File( filePath ) );
        }
        Collections.sort( noMatchKeyList );
        return noMatchKeyList;
    }
}
