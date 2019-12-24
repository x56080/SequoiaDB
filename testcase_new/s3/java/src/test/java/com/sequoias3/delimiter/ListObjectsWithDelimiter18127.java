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
 * test content: 带prefix、delimiter、start-after和maxkeys匹配查询对象元数据列表 testlink-case:
 * seqDB-18127
 *
 * @author wangkexin
 * @Date 2019.04.22
 * @version 1.00
 */

public class ListObjectsWithDelimiter18127 extends S3TestBase {
    private String bucketName = "bucket18127";
    private String[] keyList = { "dir1?test18127_1",
            "dir1??dir2??/dir3/test18127_2", "dir1?test18127_3",
            "dir1?dir2?aa?test18127_4", "dir1?dir2?aa?cc?test18127_5",
            "dir1?dir2?aa?dd?test18127_6" };
    private String prefix = "dir1?";
    private String delimiter = "?";
    private String startAfter = "dir0?";
    private int maxKeys1 = 10;
    private int maxKeys2 = 1;
    private AmazonS3 s3Client = null;
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {
        s3Client = CommLib.buildS3Client();
        // create bucket and set bucket version status
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );

        // put multiple objects
        for ( String keyName : keyList ) {
            s3Client.putObject( bucketName, keyName, "object_file18127" );
        }
    }

    @Test
    public void testGetObjectList() throws Exception {
        // 将分隔符设置为? （默认为'/'）
        DelimiterUtils.putBucketDelimiter( bucketName, delimiter );
        DelimiterUtils.checkCurrentDelimiteInfo( bucketName, delimiter );

        // 指定maxkeys大于匹配记录数
        List<String> commprefixesResult = new ArrayList<>();
        List<String> actContents = new ArrayList<>();
        ListObjectsV2Request req = new ListObjectsV2Request()
                .withBucketName( bucketName ).withPrefix( prefix )
                .withStartAfter( startAfter ).withDelimiter( delimiter )
                .withMaxKeys( maxKeys1 );
        ListObjectsV2Result result;

        result = s3Client.listObjectsV2( req );
        commprefixesResult = result.getCommonPrefixes();
        List<S3ObjectSummary> objects = result.getObjectSummaries();
        for ( S3ObjectSummary obj : objects ) {
            actContents.add( obj.getKey() );
        }
        Assert.assertFalse( result.isTruncated(),
                " commprefixes : " + commprefixesResult.toString()
                        + " contents : " + actContents.toString() );

        List<String> expCommonPrefixes = ObjectUtils
                .getCommPrefixes( keyList, prefix, delimiter );
        ObjectUtils.checkListObjectsV2Commprefixes( commprefixesResult,
                expCommonPrefixes );

        List<String> expContents = ObjectUtils
                .getKeys( keyList, prefix, delimiter );
        Collections.sort( expContents );
        Assert.assertEquals( actContents, expContents,
                "act contents : " + actContents.toString()
                        + " , exp contects : " + expContents.toString() );

        // 指定maxkeys小于匹配记录数
        List<String> commprefixesResult2 = new ArrayList<>();
        List<String> actContents2 = new ArrayList<>();
        ListObjectsV2Request req2 = new ListObjectsV2Request()
                .withBucketName( bucketName ).withPrefix( prefix )
                .withStartAfter( startAfter ).withDelimiter( delimiter )
                .withMaxKeys( maxKeys2 );
        ListObjectsV2Result result2 = null;
        int returnedNum = 1;
        do {
            result2 = s3Client.listObjectsV2( req2 );
            commprefixesResult2.addAll( result2.getCommonPrefixes() );
            List<S3ObjectSummary> objects2 = result2.getObjectSummaries();
            for ( S3ObjectSummary obj : objects2 ) {
                actContents2.add( obj.getKey() );
            }
            // 判断maxKeys是否生效
            Assert.assertEquals(
                    commprefixesResult2.size() + actContents2.size(),
                    returnedNum * maxKeys2 );
            String nextContinuationToken = result2.getNextContinuationToken();
            req2.setContinuationToken( nextContinuationToken );
            returnedNum++;
        } while ( result2.isTruncated() );

        ObjectUtils.checkListObjectsV2Commprefixes( commprefixesResult2,
                expCommonPrefixes );
        Assert.assertEquals( actContents2, expContents,
                " act contents : " + actContents2.toString()
                        + " , exp contects : " + expContents.toString() );
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
