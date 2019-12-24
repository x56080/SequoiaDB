package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.ListObjectsV2Request;
import com.amazonaws.services.s3.model.ListObjectsV2Result;
import com.amazonaws.services.s3.model.S3ObjectSummary;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * @Description seqDB-16424: To get a list of objects within a bucket.specify
 *              matching delimiter with different formats
 * @author wuyan
 * @Date 2018.11.19
 * @version 1.00
 */
public class ListObjectsWithDelimiter16424 extends S3TestBase {
    private AtomicInteger actSuccessTests = new AtomicInteger( 0 );
    private String bucketName = "bucket16424";
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * 10;
    private File localPath = null;
    private String filePath = null;
    private List<String> keyList = null;

    @DataProvider(name = "listWithDelimiterProvider")
    public Object[][] generatePageSize() {
        return new Object[][] {
                // the parameter : delimiter and matchObjectPosition
                // test a: delimiter type is letters and numbers
                new Object[] { "/test1/AZ/", 0 },
                // test b:delimiter type is special chararcter
                new Object[] { "/test*_.(d!-t'')", 1 },
                // test c:delimiter type is &@:,$=+?;ASCII
                new Object[] { "/test&@:,$=+? t_1", 2 },
                new Object[] { "\010te\065s", 3 },
                new Object[] { "/\35te\41a\57", 4 },
                // test d: delimiter type is 、^`><{}[]#%"~|
                new Object[] { "test、^`><{}[]#%\"~|_1", 5 }, };
    }

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
        String[] keyNames = { "/test1/AZ/16424.txt",
                "/test*_.(d!-t'')/16424.png", "/test&@:,$=+? t_16424",
                "\010te\065st_16424", "/\35te\41a\57st_16424",
                "test、^`><{}[]#%\"~|_16424" };
        keyList = putObjects( keyNames );
    }

    @Test(dataProvider = "listWithDelimiterProvider")
    public void testListObjects( String delimiter, int position )
            throws Exception {
        listObjectsAndCheckResult( delimiter, position );
        actSuccessTests.getAndIncrement();
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( actSuccessTests.get() == generatePageSize().length ) {
                CommLib.clearBucket( s3Client, bucketName );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            s3Client.shutdown();
        }
    }

    private void listObjectsAndCheckResult( String delimiter, int position )
            throws IOException {
        List<String> expKeyList = new ArrayList<>( keyList );
        ListObjectsV2Request request = new ListObjectsV2Request()
                .withBucketName( bucketName ).withEncodingType( "url" )
                .withDelimiter( delimiter );
        ListObjectsV2Result result = s3Client.listObjectsV2( request );
        List<String> commonPrefixes = result.getCommonPrefixes();

        // matching delimiter displays only 1 record
        Assert.assertEquals( commonPrefixes.size(), 1 );
        Assert.assertEquals( commonPrefixes.get( 0 ), delimiter );

        // objects do not match delimiter are displayed in contents,num is 5
        List<S3ObjectSummary> objects = result.getObjectSummaries();
        int contentsNums = 5;
        Assert.assertEquals( objects.size(), contentsNums );
        List<String> queryKeyList = new ArrayList<>();
        for ( S3ObjectSummary os : objects ) {
            String key = os.getKey();
            String etag = os.getETag();
            queryKeyList.add( key );
            Assert.assertEquals( etag, TestTools.getMD5( filePath ) );
        }

        // check the keyName
        expKeyList.remove( position );
        Collections.sort( expKeyList );
        Collections.sort( queryKeyList );
        Assert.assertEquals( queryKeyList, expKeyList );
    }

    private List<String> putObjects( String[] keys ) {
        List<String> keyList = new ArrayList<>();
        for ( int i = 0; i < keys.length; i++ ) {
            String keyName = keys[ i ];
            s3Client.putObject( bucketName, keyName, new File( filePath ) );
            keyList.add( keyName );
        }
        return keyList;
    }
}
