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
 * @Description seqDB-18580: To get a list by listObjectV1.set encoding-type is
 *              url. *
 * @author wuyan
 * @Date 2019.06.21
 * @version 1.00
 */
public class ListObjects18580 extends S3TestBase {
    private String bucketName = "bucket18580";
    private String[] objectNames = { "test!！_ST_18580", "test!.-(test18580.txt",
            "test%&/18580" };
    private String prefix = "test!";
    private String delimiter = ".-(test";
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
                .withDelimiter( delimiter ).withEncodingType( "url" );
        ObjectListing result = s3Client.listObjects( request );
        List<String> commprefixes = result.getCommonPrefixes();
        List<S3ObjectSummary> objects = result.getObjectSummaries();
        List<String> queryKeyList = new ArrayList<>();
        for ( S3ObjectSummary os : objects ) {
            String key = os.getKey();
            queryKeyList.add( key );
        }

        Assert.assertFalse( result.isTruncated() );
        List<String> expCommomPrefixs = new ArrayList<>();
        // !--%21, (--%28, the v1 list "test!.-(test" is "test%21.-%28test"
        expCommomPrefixs.add( "test%21.-%28test" );
        List<String> expQueryKeyList = new ArrayList<>();
        // the listObjectv1 list key: "test!！_ST_18580" is
        // "test%21%EF%BC%81_ST_18580"
        expQueryKeyList.add( "test%21%EF%BC%81_ST_18580" );
        Assert.assertEquals( commprefixes, expCommomPrefixs,
                "query commprefixs:" + commprefixes.toString() );
        Assert.assertEquals( queryKeyList, expQueryKeyList,
                "query contents:" + queryKeyList.toString() );

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        if ( runSuccess ) {
            CommLib.clearBucket( s3Client, bucketName );
        }
    }
}
