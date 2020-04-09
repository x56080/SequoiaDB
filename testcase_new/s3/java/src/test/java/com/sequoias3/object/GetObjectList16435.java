package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.ListObjectsV2Request;
import com.amazonaws.services.s3.model.ListObjectsV2Result;
import com.amazonaws.services.s3.model.S3ObjectSummary;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.ArrayList;
import java.util.List;

/**
 * test content: 带maxkeys多次查询对象元数据列表 testlink-case: seqDB-16435
 *
 * @author wangkexin
 * @Date 2018.11.16
 * @version 1.00
 */
public class GetObjectList16435 extends S3TestBase {
    private String bucketName = "bucket16435";
    private String keyName = "dir%test16435";
    private List< String > expresultList = new ArrayList< String >();
    private int objectNum = 25;
    private int anotherObjectNum = 4;
    private int maxKeys = 3;
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
            String currentKeyName = keyName + "_" + i;
            s3Client.putObject( bucketName, currentKeyName,
                    "object_file16435" + "." + i );
            expresultList.add( currentKeyName );
        }
        // put another objects
        for ( int i = 0; i < anotherObjectNum; i++ ) {
            String currentKeyName = "test16435" + "_" + i;
            s3Client.putObject( bucketName, currentKeyName,
                    "object_file16435_" + i );
            expresultList.add( currentKeyName );
        }
    }

    @Test
    public void testGetObjectList() throws Exception {
        int objectTotalNum = objectNum + anotherObjectNum;
        ListObjectsV2Request req = new ListObjectsV2Request()
                .withBucketName( bucketName ).withMaxKeys( maxKeys );
        List< S3ObjectSummary > objectSummaries = new ArrayList< S3ObjectSummary >();
        ListObjectsV2Result result;
        // currentTurn is query times
        int currentTurn = 0;

        do {
            currentTurn++;
            result = s3Client.listObjectsV2( req );
            objectSummaries.addAll( result.getObjectSummaries() );
            // if current turn is the last turn
            if ( currentTurn == Math
                    .ceil( ( double ) objectTotalNum / maxKeys ) ) {
                if ( objectTotalNum % maxKeys == 0 ) {
                    Assert.assertEquals( result.getKeyCount(), maxKeys,
                            "The result of the last round of return is not equal to the expected result" );
                } else {
                    Assert.assertEquals( result.getKeyCount(),
                            objectTotalNum % maxKeys,
                            "The result of the last round of return is not equal to the expected result" );
                }
            } else {
                Assert.assertEquals( result.getKeyCount(), maxKeys,
                        "The number of returned results is not equal to maxKeys" );
            }
            String NextContinuationToken = result.getNextContinuationToken();
            req.setContinuationToken( NextContinuationToken );
        } while ( result.isTruncated() );

        ObjectUtils.checkListObjectsV2KeyName( objectSummaries, expresultList );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        if ( runSuccess ) {
            CommLib.deleteAllObjectVersions( s3Client, bucketName );
            s3Client.deleteBucket( bucketName );
        }
    }
}
