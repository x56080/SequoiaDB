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
 * test content: 带prefix、delimiter查询设置continueation-token前后更新分隔符 testlink-case:
 * seqDB-18129
 *
 * @author wangkexin
 * @Date 2019.04.23
 * @version 1.00
 */

public class ListObjectsWithDelimiter18129 extends S3TestBase {
    private String bucketName = "bucket18129";
    private List< String > keyList = new ArrayList< String >();
    private String keyNamePrefix = "dir/dir";
    private String prefix = "dir/";
    private String delimiter[] = { "/", "?" };
    private int objectNum = 1200;
    private List< String > expCommonprefixes1 = new ArrayList< String >();
    private List< String > expCommonprefixes2 = new ArrayList< String >();
    private List< String > expContents = new ArrayList< String >( Arrays.asList(
            "dir/test18129_1", "dir/test18129_2", "dir/test18129_3" ) );
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
        for ( int i = 0; i < objectNum; i++ ) {
            String currentKey = keyNamePrefix + i + delimiter[ 0 ] + "dir" + i
                    + delimiter[ 1 ] + "test18129";
            s3Client.putObject( bucketName, currentKey, "object_file18129" );
            keyList.add( currentKey );
        }

        // 再上传若干不匹配分隔符的对象
        for ( String key : expContents ) {
            s3Client.putObject( bucketName, key, "object_file18129" );
        }

        String[] objectNames = new String[ keyList.size() ];
        expCommonprefixes1 = ObjectUtils.getCommPrefixes(
                keyList.toArray( objectNames ), prefix, delimiter[ 0 ] );
        expCommonprefixes2 = ObjectUtils.getCommPrefixes(
                keyList.toArray( objectNames ), prefix, delimiter[ 1 ] );

        Collections.sort( expCommonprefixes1 );
        Collections.sort( expCommonprefixes2 );
    }

    @Test
    public void testGetObjectList() throws Exception {
        // First query
        ListObjectsV2Request req = new ListObjectsV2Request()
                .withBucketName( bucketName ).withPrefix( prefix )
                .withDelimiter( delimiter[ 0 ] );
        ListObjectsV2Result result = s3Client.listObjectsV2( req );
        List< String > commprefixesResult = result.getCommonPrefixes();
        // 取出expCommonprefixes1从0开始的1000条commprefixes记录
        expCommonprefixes1 = expCommonprefixes1.subList( 0,
                objectOnceQueryNum );
        ObjectUtils.checkListObjectsV2Commprefixes( commprefixesResult,
                expCommonprefixes1 );

        // 将分隔符设置为? （默认为'/'）
        DelimiterUtils.putBucketDelimiter( bucketName, delimiter[ 1 ] );
        DelimiterUtils.checkCurrentDelimiteInfo( bucketName, delimiter[ 1 ] );

        // Second query
        List< String > commprefixesResult2 = new ArrayList<>();
        List< String > contentsResult = new ArrayList<>();
        String nextContinuationToken = result.getNextContinuationToken();
        ListObjectsV2Request req2 = new ListObjectsV2Request()
                .withBucketName( bucketName ).withPrefix( prefix )
                .withDelimiter( delimiter[ 1 ] )
                .withContinuationToken( nextContinuationToken );
        ListObjectsV2Result result2;
        do {
            result2 = s3Client.listObjectsV2( req2 );
            commprefixesResult2.addAll( result2.getCommonPrefixes() );
            List< S3ObjectSummary > contents = result2.getObjectSummaries();
            for ( S3ObjectSummary content : contents ) {
                contentsResult.add( content.getKey() );
            }
            String nextContinuationToken2 = result2.getNextContinuationToken();
            req2.setContinuationToken( nextContinuationToken2 );
        } while ( result2.isTruncated() );
        ObjectUtils.checkListObjectsV2Commprefixes( commprefixesResult2,
                expCommonprefixes2 );
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
