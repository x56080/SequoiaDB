package com.sequoias3.delimiter;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.ListObjectsV2Request;
import com.amazonaws.services.s3.model.ListObjectsV2Result;
import com.amazonaws.services.s3.model.S3ObjectSummary;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.DelimiterUtils;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

/**
 * @Description: 带prefix、start-after、delimiter在设置continueation-token前后匹配条件不一致
 * testlink-case: seqDB-18128
 *
 * @author wangkexin
 * @Date 2019.04.23
 * @version 1.00
 */

public class ListObjectsWithDelimiter18128 extends S3TestBase {
    private String bucketName = "bucket18128";
    private List< String > keyList = new ArrayList< String >();
    private String keyNamePrefix[] = { "dir1?dir", "dir2?dir3/dir" };
    private String prefix[] = { "dir1?", "dir2?" };
    private String delimiter = "?";
    private String startAfter[] = { "dir1?dir10?", "dir2?dir3/dir100?" };
    private int objectNum[] = { 1500, 500 };
    private List< String > expCommonPrefixes1 = new ArrayList< String >();
    private List< String > expCommonPrefixes2 = new ArrayList< String >();
    private List< String > expContents = new ArrayList< String >( Arrays.asList(
            "dir2?test18128_1", "dir2?test18128_2", "dir2?test18128_3" ) );
    private int objectOnceQueryNum = 1000;
    private AmazonS3 s3Client = null;
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        // create bucket and set bucket version status
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );

        // put multiple objects
        for ( int j = 0; j < objectNum[ 0 ]; j++ ) {
            String currentKey = keyNamePrefix[ 0 ] + j + "?test18128";
            s3Client.putObject( bucketName, currentKey, "object_file18128" );
            keyList.add( currentKey );
        }

        for ( int j = 0; j < objectNum[ 1 ]; j++ ) {
            String currentKey = keyNamePrefix[ 1 ] + j + "?test18128";
            s3Client.putObject( bucketName, currentKey, "object_file18128" );
            keyList.add( currentKey );
        }

        // 再上传若干不匹配分隔符的对象
        for ( String key : expContents ) {
            s3Client.putObject( bucketName, key, "object_file18128" );
        }

        String[] objectNames = new String[ keyList.size() ];
        expCommonPrefixes1 = ObjectUtils.getCommPrefixes(
                keyList.toArray( objectNames ), prefix[ 0 ], delimiter );
        expCommonPrefixes2 = ObjectUtils.getCommPrefixes(
                keyList.toArray( objectNames ), prefix[ 1 ], delimiter );

        Collections.sort( expCommonPrefixes1 );
        Collections.sort( expCommonPrefixes2 );
    }

    @Test
    public void testGetObjectList() throws Exception {
        // 将分隔符设置为? （默认为'/'）
        DelimiterUtils.putBucketDelimiter( bucketName, delimiter );
        DelimiterUtils.checkCurrentDelimiteInfo( bucketName, delimiter );

        // First query
        ListObjectsV2Request req = new ListObjectsV2Request()
                .withBucketName( bucketName ).withPrefix( prefix[ 0 ] )
                .withDelimiter( delimiter ).withStartAfter( startAfter[ 0 ] );
        ListObjectsV2Result result = s3Client.listObjectsV2( req );
        List< String > commprefixesResult = result.getCommonPrefixes();
        // 取出expCommonPrefixes1从startAfter开始的1000条commprefixes记录
        int startIndex = expCommonPrefixes1.indexOf( startAfter[ 0 ] ) + 1;
        expCommonPrefixes1 = expCommonPrefixes1.subList( startIndex,
                startIndex + objectOnceQueryNum );
        ObjectUtils.checkListObjectsV2Commprefixes( commprefixesResult,
                expCommonPrefixes1 );
        Assert.assertEquals( result.getObjectSummaries().size(), 0 );

        // Second query
        String nextContinuationToken = result.getNextContinuationToken();
        ListObjectsV2Request req2 = new ListObjectsV2Request()
                .withBucketName( bucketName ).withPrefix( prefix[ 1 ] )
                .withDelimiter( delimiter ).withStartAfter( startAfter[ 1 ] )
                .withContinuationToken( nextContinuationToken );
        ListObjectsV2Result result2 = s3Client.listObjectsV2( req2 );
        List< String > commprefixesResult2 = result2.getCommonPrefixes();
        List< String > contentsResult = new ArrayList<>();
        List< S3ObjectSummary > contents = result2.getObjectSummaries();
        for ( S3ObjectSummary content : contents ) {
            contentsResult.add( content.getKey() );
        }
        expCommonPrefixes2 = expCommonPrefixes2.subList(
                expCommonPrefixes2.indexOf( startAfter[ 1 ] ) + 1,
                expCommonPrefixes2.size() );
        ObjectUtils.checkListObjectsV2Commprefixes( commprefixesResult2,
                expCommonPrefixes2 );
        Assert.assertEquals( contentsResult, expContents,
                "contentsResult :" + contentsResult.toString()
                        + ", expContents :" + expContents.toString() );

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                CommLib.deleteAllObjectVersions( s3Client, bucketName );
                s3Client.deleteBucket( bucketName );
            }
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }
}
