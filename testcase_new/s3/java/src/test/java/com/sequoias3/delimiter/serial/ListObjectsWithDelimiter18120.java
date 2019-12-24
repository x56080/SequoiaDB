package com.sequoias3.delimiter.serial;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.ListObjectsV2Request;
import com.amazonaws.services.s3.model.ListObjectsV2Result;
import com.amazonaws.services.s3.model.S3ObjectSummary;
import com.sequoias3.delimiter.DelimiterConfiguration;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.DelimiterUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.net.URLDecoder;
import java.net.URLEncoder;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * @Description seqDB-18120: To get a list of objects within a bucket.specify
 * @author wuyan
 * @version 1.00
 * matching delimiter with different formats
 * @Date 2019.4.23
 */
public class ListObjectsWithDelimiter18120 extends S3TestBase {
    private AtomicInteger actSuccessTests = new AtomicInteger( 0 );
    private String bucketName = "bucket18120";
    private String encodingType = "utf-8";
    private AmazonS3 s3Client = null;
    private List<String> keyList = null;

    public static void updateDelimiterSuccessAgain( String bucketName,
            String delimiter, String accessKeyId, String encodingType )
            throws Exception {
        int interval = 10000;
        int tryTimes = 300;
        while ( tryTimes-- > 0 ) {
            try {
                DelimiterUtils.putBucketDelimiterWithEncodingType( bucketName,
                        delimiter, accessKeyId, encodingType );
                return;
            } catch ( AmazonS3Exception e ) {
                if ( e.getStatusCode() != 409 ) {
                    throw e;
                }
                Thread.sleep( interval );
            }
        }
        throw new Exception( "update delimiter fail,delimiter = " + delimiter );
    }

    @BeforeClass
    private void setUp() throws IOException {
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
        String[] keyNames = { "/test1/AZ/18120_test1.txt",
                "/test*_.(d!-t'')/18120_test2.png",
                "/test&@:,$=+? t_18120_test3", "\040te\065st_18120_test4",
                "/\035te\41a\57st_18120_test5",
                "test、^`><{}[]#%\"~|_18120_test6" };
        keyList = putObjects( keyNames );
    }

    @DataProvider(name = "listWithDelimiterProvider")
    public Object[][] generatePageSize() throws UnsupportedEncodingException {
        return new Object[][] {
                // the parameter : delimiter and matchObjectPosition
                // test a: delimiter type is letters and numbers
                new Object[] { URLEncoder.encode( "/test1/AZ/", encodingType ),
                        0 },
                // test b:delimiter type is special chararcter
                new Object[] {
                        URLEncoder.encode( "/test*_.(d!-t'')", encodingType ),
                        1 },
                // test c:delimiter type is &@:,$=+?;ASCII
                new Object[] {
                        URLEncoder.encode( "/test&@:,$=+? t_1", encodingType ),
                        2 },
                // http://jira:8080/browse/SEQUOIADBMAINSTREAM-4392
                new Object[] { URLEncoder.encode( "\040te\065s", encodingType ),
                        3 }, new Object[] {
                URLEncoder.encode( "/\035te\41a\57", encodingType ), 4 },
                // test d: delimiter type is 、^`><{}[]#%"~|
                new Object[] { URLEncoder.encode( "test、^`><{}[]#%\"~|_1",
                        encodingType ), 5 } };
    }

    @Test(dataProvider = "listWithDelimiterProvider")
    public void testListObjects( String delimiter, int position )
            throws Exception {
        updateDelimiterSuccessAgain( bucketName, delimiter,
                S3TestBase.s3AccessKeyId, "url" );
        DelimiterConfiguration currDelimiter = DelimiterUtils
                .getDelimiterWithEncodingType( bucketName,
                        S3TestBase.s3AccessKeyId, "url" );
        Assert.assertEquals(
                URLDecoder.decode( currDelimiter.getDelimiter(), encodingType ),
                URLDecoder.decode( delimiter, encodingType ) );
        Assert.assertEquals( currDelimiter.getStatus(), "Normal" );
        listObjectsAndCheckResult( delimiter, position );
        actSuccessTests.getAndIncrement();
    }

    @AfterClass
    private void tearDown() throws UnsupportedEncodingException {
        try {
            if ( actSuccessTests.get() == generatePageSize().length ) {
                CommLib.clearBucket( s3Client, bucketName );
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
                .withDelimiter( URLDecoder.decode( delimiter, encodingType ) );
        ListObjectsV2Result result = s3Client.listObjectsV2( request );
        List<String> commonPrefixes = result.getCommonPrefixes();
        // matching delimiter displays only 1 record
        Assert.assertEquals( commonPrefixes.size(), 1 );
        Assert.assertEquals( commonPrefixes.get( 0 ),
                URLDecoder.decode( delimiter, "utf-8" ) );
        // objects do not match delimiter are displayed in contents,num is 5
        List<S3ObjectSummary> objects = result.getObjectSummaries();
        int contentsNums = 5;
        Assert.assertEquals( objects.size(), contentsNums );
        List<String> queryKeyList = new ArrayList<>();
        for ( S3ObjectSummary os : objects ) {
            String key = os.getKey();
            queryKeyList.add( key );
        }
        // check the keyName
        expKeyList.remove( position );
        Collections.sort( expKeyList );
        Collections.sort( queryKeyList );
        Assert.assertEquals( queryKeyList, expKeyList,
                "queryKeyList:" + queryKeyList.toString() + " expKeyList:"
                        + expKeyList.toString() );
    }

    private List<String> putObjects( String[] keys ) {
        List<String> keyList = new ArrayList<>();
        for ( int i = 0; i < keys.length; i++ ) {
            String keyName = keys[ i ];
            s3Client.putObject( bucketName, keyName, "testcontext18120_" + i );
            keyList.add( keyName );
        }
        return keyList;
    }
}
