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
import org.testng.annotations.Test;

import java.util.ArrayList;
import java.util.List;

/**
 * @Description seqDB-18579: To get a list by listObjectV1.Multiple query
 *              results match the same record in commprefix.
 * @author wuyan
 * @Date 2019.06.21
 * @version 1.00
 */
public class ListObjects18579 extends S3TestBase {
    private String bucketName = "bucket18579";
    private String[] objectNames = { "/aa?aa?test1_18579", "/aa/bb/test2_18579",
            "/aa/bb/test3_18579", "/aa_test4_18579" };
    private String prefix = "/aa";
    private String delimiter = "/";
    private int maxKeys = 2;
    private AmazonS3 s3Client = null;
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );

        for ( int i = 0; i < objectNames.length; i++ ) {
            String keyName = objectNames[ i ];
            s3Client.putObject( bucketName, keyName, "testcontent_" + keyName );
        }
    }

    @Test
    public void testListObject() {
        ListObjectsRequest request = new ListObjectsRequest()
                .withBucketName( bucketName ).withPrefix( prefix )
                .withDelimiter( delimiter ).withMaxKeys( maxKeys );
        ObjectListing result;
        // first list
        result = s3Client.listObjects( request );
        List<String> firstCommprefixes = result.getCommonPrefixes();
        List<S3ObjectSummary> objects = result.getObjectSummaries();
        List<String> firstQueryKeyList = new ArrayList<>();
        String key = "";
        for ( S3ObjectSummary os : objects ) {
            key = os.getKey();
            firstQueryKeyList.add( key );
        }

        // check the first list result
        Assert.assertTrue( result.isTruncated() );
        List<String> expFirstCommomPrefixs = new ArrayList<>();
        expFirstCommomPrefixs.add( "/aa/" );
        List<String> expFirstQueryKeyList = new ArrayList<>();
        expFirstQueryKeyList.add( "/aa?aa?test1_18579" );
        Assert.assertEquals( firstCommprefixes, expFirstCommomPrefixs,
                "query commprefixs:" + firstCommprefixes.toString() );
        Assert.assertEquals( firstQueryKeyList, expFirstQueryKeyList,
                "query contents:" + firstQueryKeyList.toString() );

        String keyName1 = "/aa/bb/test5_18579";
        String keyName2 = "/aa_test6_18579";
        String keyName3 = "/aa/test7_18579";
        s3Client.putObject( bucketName, keyName1, "test" );
        s3Client.putObject( bucketName, keyName2, "test" );
        s3Client.putObject( bucketName, keyName3, "test" );
        // second list, match the new putObject("/aa/bb/test5_18579")
        String marker = result.getNextMarker();
        request.setMarker( marker );
        result = s3Client.listObjects( request );
        List<String> secondCommprefixes = result.getCommonPrefixes();
        List<S3ObjectSummary> objects2 = result.getObjectSummaries();
        List<String> secondQueryKeyList = new ArrayList<>();
        for ( S3ObjectSummary os : objects2 ) {
            String key2 = os.getKey();
            secondQueryKeyList.add( key2 );
        }

        // check the second list result
        Assert.assertFalse( result.isTruncated() );
        List<String> expSecondCommomPrefixs = new ArrayList<>();
        List<String> expSecondQueryKeyList = new ArrayList<>();
        expSecondQueryKeyList.add( "/aa_test4_18579" );
        expSecondQueryKeyList.add( keyName2 );
        Assert.assertEquals( secondCommprefixes, expSecondCommomPrefixs,
                "second query commprefixs:" + secondCommprefixes.toString() );
        Assert.assertEquals( secondQueryKeyList, expSecondQueryKeyList,
                "second query contents:" + secondQueryKeyList.toString() );

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        if ( runSuccess ) {
            CommLib.clearBucket( s3Client, bucketName );
        }
    }
}
