package com.sequoias3.delimiter;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.ListObjectsV2Request;
import com.amazonaws.services.s3.model.ListObjectsV2Result;
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
 * @Description: 两次查询间隔时间超过上下文生命周期时间 testlink-case: seqDB-18132
 *
 * @author wangkexin
 * @Date 2019.04.24
 * @version 1.00
 */
@Test(groups = "contextlifecycleconf")
public class ListObjectsWithDelimiter18132 extends S3TestBase {
    private String bucketName = "bucket18132";
    private String keyNamePrefix = "dir/dir";
    private String[] objectNames = new String[ 10 ];
    private String delimiter = "?";
    private int maxkeys = 2;
    private List< String > expCommonprefixes = new ArrayList< String >();
    private AmazonS3 s3Client = null;
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {
        s3Client = CommLib.buildS3Client();
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );

        for ( int i = 0; i < objectNames.length; i++ ) {
            String currentKeyName = keyNamePrefix + i + delimiter + "16438";
            s3Client.putObject( bucketName, currentKeyName,
                    "object_file18132" );
            objectNames[ i ] = currentKeyName;
        }
    }

    @Test
    public void testGetObjectList() throws Exception {
        // 将分隔符设置为? （默认为'/'）
        DelimiterUtils.putBucketDelimiter( bucketName, delimiter );
        DelimiterUtils.checkCurrentDelimiteInfo( bucketName, delimiter );

        expCommonprefixes = ObjectUtils.getCommPrefixes( objectNames, "",
                delimiter );
        Collections.sort( expCommonprefixes );
        ListObjectsV2Request req = new ListObjectsV2Request()
                .withBucketName( bucketName ).withDelimiter( delimiter )
                .withMaxKeys( maxkeys );
        ListObjectsV2Result result;

        result = s3Client.listObjectsV2( req );
        List< String > commprefixesResult = result.getCommonPrefixes();

        // check result
        List< String > tmpCommonprefixes = expCommonprefixes.subList( 0,
                maxkeys );
        ObjectUtils.checkListObjectsV2Commprefixes( commprefixesResult,
                tmpCommonprefixes );

        Thread.sleep( 3 * 60 * 1000 );
        // second query
        String nextContinuationToken = result.getNextContinuationToken();
        ListObjectsV2Request req2 = new ListObjectsV2Request()
                .withBucketName( bucketName )
                .withContinuationToken( nextContinuationToken );
        try {
            s3Client.listObjectsV2( req2 );
            Assert.fail( "exp fail but found success" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "InvalidArgument" );
        }

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
