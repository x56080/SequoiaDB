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
 * test content: 带prefix、start-after、delimiter匹配查询对象元数据列表（多次查询） testlink-case:
 * seqDB-18126
 *
 * @author wangkexin
 * @Date 2019.04.16
 * @version 1.00
 */

public class ListObjectsWithDelimiter18126 extends S3TestBase {
    private String bucketName = "bucket18126";
    private String keyName = "%dir%dir";
    private String prefix = "%dir%";
    private String delimiter = "%";
    private String startAfter = "%dir%dir10%";
    private List< String > expCommprefixes = new ArrayList< String >();
    private List< String > expContents = new ArrayList< String >( Arrays.asList(
            "%dir%test18126_1", "%dir%test18126_2", "%dir%test18126_3" ) );
    private int objectTotalNum = 2000;
    private AmazonS3 s3Client = null;
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {
        s3Client = CommLib.buildS3Client();
        // create bucket and set bucket version status
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );

        // put multiple objects
        for ( int i = 0; i < objectTotalNum; i++ ) {
            String currentKeyName = keyName + i + "%18126";
            s3Client.putObject( bucketName, currentKeyName,
                    "object_file18126" );
            String commprefix = currentKeyName.substring( 0,
                    currentKeyName.lastIndexOf( delimiter ) + 1 );
            expCommprefixes.add( commprefix );
        }
        Collections.sort( expCommprefixes );

        // 再上传若干不匹配分隔符的对象
        for ( String key : expContents ) {
            s3Client.putObject( bucketName, key, "object_file18126" );
        }
    }

    @Test
    public void testGetObjectList() throws Exception {
        // 将分隔符设置为% （默认为'/'）
        DelimiterUtils.putBucketDelimiter( bucketName, delimiter );
        DelimiterUtils.checkCurrentDelimiteInfo( bucketName, delimiter );

        List< String > commprefixesResult = new ArrayList<>();
        List< String > contentsResult = new ArrayList<>();
        ListObjectsV2Request req = new ListObjectsV2Request()
                .withBucketName( bucketName ).withPrefix( prefix )
                .withDelimiter( delimiter ).withStartAfter( startAfter );
        ListObjectsV2Result result;

        int queryNum = 0;
        do {
            queryNum++;
            result = s3Client.listObjectsV2( req );
            commprefixesResult.addAll( result.getCommonPrefixes() );
            List< S3ObjectSummary > contents = result.getObjectSummaries();
            for ( S3ObjectSummary content : contents ) {
                contentsResult.add( content.getKey() );
            }
            String nextContinuationToken = result.getNextContinuationToken();
            req.setContinuationToken( nextContinuationToken );
        } while ( result.isTruncated() );

        // 保证多次查询，验证nextContinuationToken有效
        Assert.assertEquals( queryNum, 2 );

        // expresultList are stored after 'startAfter'
        // subList[int,int)
        expCommprefixes = expCommprefixes.subList(
                expCommprefixes.indexOf( startAfter ) + 1,
                expCommprefixes.size() );
        ObjectUtils.checkListObjectsV2Commprefixes( commprefixesResult,
                expCommprefixes );
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
