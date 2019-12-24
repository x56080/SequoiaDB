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
import java.util.Collections;
import java.util.List;

/**
 * test content: 指定nextContinuationToken匹配记录被删除，查询列表元数据 testlink-case:
 * seqDB-18131
 *
 * @author wangkexin
 * @Date 2019.04.23
 * @version 1.00
 */

public class ListObjectsWithDelimiter18131 extends S3TestBase {
    private String bucketName = "bucket18131";
    private String[] objectNames = { "dir1/test18131_1",
            "dir1/dir2/test18131_2", "dir/aa/test18131_3", "test18131_4" };
    private String delimiter = "test";
    private int maxkeys = 2;
    private List<String> expCommonprefixes = new ArrayList<String>();
    private AmazonS3 s3Client = null;
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {
        s3Client = CommLib.buildS3Client();
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );

        for ( int i = 0; i < objectNames.length; i++ ) {
            s3Client.putObject( bucketName, objectNames[ i ],
                    "object_file18131" );
        }
    }

    @Test
    public void testGetObjectList() throws Exception {
        // 将分隔符设置为test （默认为'/'）
        DelimiterUtils.putBucketDelimiter( bucketName, delimiter );
        DelimiterUtils.checkCurrentDelimiteInfo( bucketName, delimiter );

        expCommonprefixes = ObjectUtils
                .getCommPrefixes( objectNames, "", delimiter );
        Collections.sort( expCommonprefixes );
        List<String> commprefixesResult = new ArrayList<>();
        ListObjectsV2Request req = new ListObjectsV2Request()
                .withBucketName( bucketName ).withDelimiter( delimiter )
                .withMaxKeys( maxkeys );
        ListObjectsV2Result result;

        result = s3Client.listObjectsV2( req );
        commprefixesResult.addAll( result.getCommonPrefixes() );

        // 删除下一个匹配到的记录对应的对象，并且将expCommonprefixes中对应的值删除
        String tobeDeleteKey = getNextRecordKey( Integer.valueOf( maxkeys ) );
        s3Client.deleteObject( bucketName, tobeDeleteKey );
        expCommonprefixes.remove( maxkeys );

        String nextContinuationToken = result.getNextContinuationToken();
        ListObjectsV2Request req2 = new ListObjectsV2Request()
                .withBucketName( bucketName ).withDelimiter( delimiter )
                .withMaxKeys( maxkeys )
                .withContinuationToken( nextContinuationToken );
        ListObjectsV2Result result2 = s3Client.listObjectsV2( req2 );
        commprefixesResult.addAll( result2.getCommonPrefixes() );

        List<String> contentsResult = new ArrayList<>();
        List<S3ObjectSummary> contents = result.getObjectSummaries();
        for ( S3ObjectSummary content : contents ) {
            contentsResult.add( content.getKey() );
        }

        List<String> expContents = ObjectUtils
                .getKeys( objectNames, "", delimiter );

        // check result
        ObjectUtils.checkListObjectsV2Commprefixes( commprefixesResult,
                expCommonprefixes );
        Assert.assertEquals( contentsResult, expContents );
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

    private String getNextRecordKey( int deleteIndex ) {
        // 获取下次返回的commonprefix值
        String nextCommprefix = expCommonprefixes.get( deleteIndex );
        String nextRecordKey = "";
        // 找到objectNames中与nextCommprefix对应的对象名
        for ( int i = 0; i < objectNames.length; i++ ) {
            if ( objectNames[ i ].startsWith( nextCommprefix ) ) {
                nextRecordKey = objectNames[ i ];
                break;
            }
        }
        return nextRecordKey;
    }
}
