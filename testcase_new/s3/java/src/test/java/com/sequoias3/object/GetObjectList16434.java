package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.ListObjectsV2Request;
import com.amazonaws.services.s3.model.ListObjectsV2Result;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/**
 * test content: 带prefix、start-after、delimiter在设置continueation-token前后匹配条件不一致
 * testlink-case: seqDB-16434
 *
 * @author wangkexin
 * @Date 2018.11.16
 * @version 1.00
 */
public class GetObjectList16434 extends S3TestBase {
    private String bucketName = "bucket16434";
    private String keyName = "/dir/dir";
    private String[] prefix = { "/dir/", "/dir/dir1200/" };
    private String delimiter = "/";
    private String startAfter[] = { "/dir/dir10/", "/dir/dir1200/test10/" };
    private List< String > expresultList1 = new ArrayList< String >( 1000 );
    private List< String > expresultList2 = new ArrayList< String >( 1000 );
    private int objectNum = 1500;
    private int anotherObjectNum = 500;
    private int objectOnceQueryNum = 1000;
    private AmazonS3 s3Client = null;
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {
        s3Client = CommLib.buildS3Client();
        // create bucket and set bucket version status
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );

        // put multiple objects
        for ( int i = 0; i < objectNum; i++ ) {
            String currentKeyName = keyName + i + "/16434";
            s3Client.putObject( bucketName, currentKeyName,
                    "object_file16434" );
            String commprefix = currentKeyName.substring( 0,
                    currentKeyName.lastIndexOf( delimiter ) + 1 );
            expresultList1.add( commprefix );
        }
        // put another multiple objects
        for ( int i = 0; i < anotherObjectNum; i++ ) {
            String currentKeyName = keyName + "1/test" + i + "/16434";
            s3Client.putObject( bucketName, currentKeyName,
                    "object_file16434" );
        }
        for ( int i = 0; i < anotherObjectNum; i++ ) {
            String currentKeyName = keyName + "1200/test" + i + "/16434";
            s3Client.putObject( bucketName, currentKeyName,
                    "object_file16434" );
            String commprefix = currentKeyName.substring( 0,
                    currentKeyName.lastIndexOf( delimiter ) + 1 );
            expresultList2.add( commprefix );
        }

        Collections.sort( expresultList1 );
        Collections.sort( expresultList2 );
    }

    @Test
    public void testGetObjectList() throws Exception {
        // First query
        ListObjectsV2Request req = new ListObjectsV2Request()
                .withBucketName( bucketName ).withPrefix( prefix[ 0 ] )
                .withDelimiter( delimiter ).withStartAfter( startAfter[ 0 ] );
        ListObjectsV2Result result = s3Client.listObjectsV2( req );
        List< String > commprefixesResult = result.getCommonPrefixes();
        expresultList1 = expresultList1.subList(
                expresultList1.indexOf( startAfter[ 0 ] ) + 1,
                expresultList1.size() );
        checkListObjectsV2Result( commprefixesResult, expresultList1,
                objectOnceQueryNum );

        // Second query
        String nextContinuationToken = result.getNextContinuationToken();
        ListObjectsV2Request req2 = new ListObjectsV2Request()
                .withBucketName( bucketName ).withPrefix( prefix[ 1 ] )
                .withDelimiter( delimiter ).withStartAfter( startAfter[ 1 ] )
                .withContinuationToken( nextContinuationToken );
        ListObjectsV2Result result2 = s3Client.listObjectsV2( req2 );
        List< String > commprefixesResult2 = result2.getCommonPrefixes();
        expresultList2 = expresultList2.subList(
                expresultList2.indexOf( startAfter[ 1 ] ) + 1,
                expresultList2.size() );
        checkListObjectsV2Result( commprefixesResult2, expresultList2,
                expresultList2.size() );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        if ( runSuccess ) {
            CommLib.deleteAllObjectVersions( s3Client, bucketName );
            s3Client.deleteBucket( bucketName );
        }
    }

    private void checkListObjectsV2Result( List< String > resultList,
            List< String > expresultList, int keyCount ) {
        Assert.assertEquals( resultList.size(), keyCount,
                "The expected results do not match the actual number of returns" );
        for ( int i = 0; i < resultList.size(); i++ ) {
            Assert.assertEquals( resultList.get( i ), expresultList.get( i ),
                    "commonPrefixes is wrong" );
        }
    }
}
