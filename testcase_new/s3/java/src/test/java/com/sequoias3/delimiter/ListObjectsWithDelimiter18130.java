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
import java.util.List;

/**
 * test content: 带delimiter和maxkeys多次查询对象元数据列表 testlink-case: seqDB-18130
 *
 * @author wangkexin
 * @Date 2019.04.23
 * @version 1.00
 */

public class ListObjectsWithDelimiter18130 extends S3TestBase {
    private String bucketName = "bucket18130";
    private String[] objectNames = { "dir1?test18130_1",
            "dir1??dir2??/dir3/test18130_2", "dir1?test18130_3",
            "dir1?dir2?aa?test18130_4", "dir1?dir2?aa?cc?test18130_5",
            "dir1?dir2?aa?dd?test18130_6", "dir1?dir2?aa?ee?test18130_7",
            "dir1?dir2?aa?dd?cctest18130_8" };
    private String delimiter = "tes";
    private int maxkeys = 2;
    private List<String> expCommprefixes = new ArrayList<String>();
    private AmazonS3 s3Client = null;
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {
        s3Client = CommLib.buildS3Client();
        // create bucket and set bucket version status
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );

        // put multiple objects
        for ( int i = 0; i < objectNames.length; i++ ) {
            s3Client.putObject( bucketName, objectNames[ i ],
                    "object_file18130" );
        }
    }

    @Test
    public void testGetObjectList() throws Exception {
        // 将分隔符设置为tes （默认为'/'）
        DelimiterUtils.putBucketDelimiter( bucketName, delimiter );
        DelimiterUtils.checkCurrentDelimiteInfo( bucketName, delimiter );

        expCommprefixes = ObjectUtils
                .getCommPrefixes( objectNames, "", delimiter );
        int listReturnTimes = ( int ) Math
                .ceil( ( double ) expCommprefixes.size() / maxkeys );
        int lastReturnNum = expCommprefixes.size() % maxkeys;
        int times = 0;
        List<String> commprefixesResult = new ArrayList<>();
        List<String> contentsResult = new ArrayList<>();
        ListObjectsV2Request req = new ListObjectsV2Request()
                .withBucketName( bucketName ).withDelimiter( delimiter )
                .withMaxKeys( maxkeys );
        ListObjectsV2Result result;

        do {
            times++;
            result = s3Client.listObjectsV2( req );
            if ( times == listReturnTimes && lastReturnNum != 0 ) {
                Assert.assertEquals( result.getKeyCount(), lastReturnNum,
                        "return num is wrong in last round." );
            } else {
                Assert.assertEquals( result.getKeyCount(), maxkeys,
                        "return num is wrong" );
            }

            List<S3ObjectSummary> contents = result.getObjectSummaries();
            for ( S3ObjectSummary content : contents ) {
                contentsResult.add( content.getKey() );
            }
            commprefixesResult.addAll( result.getCommonPrefixes() );
            String nextContinuationToken = result.getNextContinuationToken();
            req.setContinuationToken( nextContinuationToken );
        } while ( result.isTruncated() );
        ObjectUtils.checkListObjectsV2Commprefixes( commprefixesResult,
                expCommprefixes );
        Assert.assertEquals( contentsResult.size(), 0,
                "contentsResult : " + contentsResult.toString() );
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
