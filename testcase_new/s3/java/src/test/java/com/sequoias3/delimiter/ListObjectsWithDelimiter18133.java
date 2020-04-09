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
 * test content: 多次查询结果在commprefix中有相同记录 testlink-case: seqDB-18133
 *
 * @author wangkexin
 * @Date 2019.04.24
 * @version 1.00
 */

public class ListObjectsWithDelimiter18133 extends S3TestBase {
    private String bucketName = "bucket18133";
    private String[] objectNames = { "/aa/bb/test18133_1", "/aa/bb/test18133_2",
            "/bb/cc/test18133_3", "/bb/cc/test18133_4",
            "/bb/cc/test/test18133_5", "/cc/bb/test18133_6、test18133_5",
            "18133.txt" };
    private String delimiter = "te";
    private int maxkeys = 2;
    private List< String > expCommonprefixes = new ArrayList< String >();
    private AmazonS3 s3Client = null;
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {
        s3Client = CommLib.buildS3Client();
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );

        // put multiple objects
        for ( int i = 0; i < objectNames.length; i++ ) {
            s3Client.putObject( bucketName, objectNames[ i ],
                    "object_file18133" );
        }
    }

    @Test
    public void testGetObjectList() throws Exception {
        // 将分隔符设置为te（默认为'/'）
        DelimiterUtils.putBucketDelimiter( bucketName, delimiter );
        DelimiterUtils.checkCurrentDelimiteInfo( bucketName, delimiter );

        ListObjectsV2Request req = new ListObjectsV2Request()
                .withBucketName( bucketName ).withDelimiter( delimiter )
                .withMaxKeys( maxkeys );
        ListObjectsV2Result result = s3Client.listObjectsV2( req );
        List< String > commprefixesResult = result.getCommonPrefixes();

        // check result
        expCommonprefixes = ObjectUtils.getCommPrefixes( objectNames, "",
                delimiter );
        List< String > tmpCommonprefixes = expCommonprefixes.subList( 0,
                maxkeys );
        ObjectUtils.checkListObjectsV2Commprefixes( commprefixesResult,
                tmpCommonprefixes );

        String nextContinuationToken = result.getNextContinuationToken();
        req.setContinuationToken( nextContinuationToken );
        result = s3Client.listObjectsV2( req );
        List< String > commprefixesResult2 = result.getCommonPrefixes();
        List< String > contentsResult = new ArrayList<>();
        List< S3ObjectSummary > contents = result.getObjectSummaries();
        for ( S3ObjectSummary content : contents ) {
            contentsResult.add( content.getKey() );
        }

        // finally check result
        List< String > tmpCommonprefixes2 = expCommonprefixes.subList( maxkeys,
                expCommonprefixes.size() );
        ObjectUtils.checkListObjectsV2Commprefixes( commprefixesResult2,
                tmpCommonprefixes2 );
        List< String > expContents = ObjectUtils.getKeys( objectNames, "",
                delimiter );
        Assert.assertEquals( contentsResult, expContents );
        Assert.assertEquals( result.isTruncated(), false );
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