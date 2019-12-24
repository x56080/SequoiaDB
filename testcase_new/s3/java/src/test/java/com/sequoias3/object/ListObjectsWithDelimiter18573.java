package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.ListObjectsRequest;
import com.amazonaws.services.s3.model.ObjectListing;
import com.amazonaws.services.s3.model.S3ObjectSummary;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * @Description seqDB-18573: To get a list by listObjectV1.specify matching
 *              delimiter with different formats;
 * @author wuyan
 * @Date 2019.6.20
 * @version 1.00
 */
public class ListObjectsWithDelimiter18573 extends S3TestBase {
    private AtomicInteger actSuccessTests = new AtomicInteger( 0 );
    private String bucketName = "bucket18573";
    private AmazonS3 s3Client = null;
    private List<String> keyList = null;

    @DataProvider(name = "listWithDelimiterProvider")
    public Object[][] generatePageSize() {
        return new Object[][] {
                // the parameter : delimiter and matchObjectPosition
                // test a: delimiter type is letters and numbers
                new Object[] { "/test1/AZ/az09", 3 },
                // test b:delimiter type is special character
                new Object[] { "/test*_.(d!-t'')", 2 },
                // test c:delimiter type is &@:,$=+?;
                new Object[] { "/test&@:,$=+? t_1", 1 },
                // test c:delimiter type is ASCII（1-31），127
                new Object[] { "\0177\01te\037s", 0 },
                // test d: delimiter type is 、^`><{}[]#%"~|
                new Object[] { "test、^`><{}[]#%\"~|_1", 4 } };
    }

    @BeforeClass
    private void setUp() {
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
        String[] keyNames = { "\0177\01te\037st_18573_test1",
                "/test&@:,$=+? t_18573_test2",
                "/test*_.(d!-t'')/18573_test3.png",
                "/test1/AZ/az09/18573_test4.txt",
                "test、^`><{}[]#%\"~|_18573_test5" };
        keyList = putObjects( keyNames );
    }

    @Test(dataProvider = "listWithDelimiterProvider")
    public void testListObjects( String delimiter, int position )
            throws Exception {
        listObjectV1AndCheckResult( delimiter, position );
        actSuccessTests.getAndIncrement();
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( actSuccessTests.get() == generatePageSize().length ) {
                CommLib.clearBucket( s3Client, bucketName );
            }
        } finally {
            s3Client.shutdown();
        }
    }

    private void listObjectV1AndCheckResult( String delimiter, int position ) {
        ListObjectsRequest request = new ListObjectsRequest()
                .withBucketName( bucketName ).withDelimiter( delimiter );
        ObjectListing result = s3Client.listObjects( request );
        List<String> commonPrefixes = result.getCommonPrefixes();
        // matching delimiter displays only 1 record
        Assert.assertEquals( commonPrefixes.size(), 1 );
        Assert.assertEquals( commonPrefixes.get( 0 ), delimiter );

        // objects do not match delimiter are displayed in contents,num is 4
        List<S3ObjectSummary> objects = result.getObjectSummaries();
        int contentsNums = 4;
        Assert.assertEquals( objects.size(), contentsNums );
        List<String> queryKeyList = new ArrayList<>();
        for ( S3ObjectSummary os : objects ) {
            String key = os.getKey();
            queryKeyList.add( key );
        }

        // check the keyName
        List<String> expKeyList = new ArrayList<>( keyList );
        expKeyList.remove( position );
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
        Collections.sort( keyList );
        return keyList;
    }
}
