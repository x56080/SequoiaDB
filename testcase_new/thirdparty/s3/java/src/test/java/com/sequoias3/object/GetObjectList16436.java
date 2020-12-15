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
 * @Description: 带start-after和maxkeys查询对象元数据列表 testlink-case: seqDB-16436
 *
 * @author wangkexin
 * @Date 2018.11.16
 * @version 1.00
 */
public class GetObjectList16436 extends S3TestBase {
    private String bucketName = "bucket16436";
    private String keyName = "%%dir";
    private List< String > keyNameList = new ArrayList< String >( 10 );
    private int objectTotalNum = 10;
    private AmazonS3 s3Client = null;
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {
        s3Client = CommLib.buildS3Client();
        // create bucket and set bucket version status
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );

        // put multiple objects
        for ( int i = 0; i < objectTotalNum; i++ ) {
            String currentKeyName = keyName + i + "_16436";
            s3Client.putObject( bucketName, currentKeyName,
                    "object_file16436" );
            keyNameList.add( currentKeyName );
        }
    }

    @Test
    public void testGetObjectList() throws Exception {
        int maxKeys = 2;
        // startAfter match the first record
        ListObjectsV2Request req = new ListObjectsV2Request()
                .withBucketName( bucketName ).withStartAfter( "%%dir0_16436" )
                .withMaxKeys( maxKeys );
        List< S3ObjectSummary > objectSummaries = new ArrayList< S3ObjectSummary >();
        ListObjectsV2Result result;
        // currentTurn is query times
        int queryTime = 0;
        int currentTotalNum = objectTotalNum - 1;
        keyNameList.remove( 0 );
        do {
            queryTime++;
            result = s3Client.listObjectsV2( req );
            objectSummaries.addAll( result.getObjectSummaries() );
            // if current turn is the last turn
            if ( queryTime == Math
                    .ceil( ( double ) currentTotalNum / maxKeys ) ) {
                Assert.assertEquals( result.getKeyCount(), 1,
                        "The result of the last round of return is not equal to the expected result" );
            } else {
                Assert.assertEquals( result.getKeyCount(), maxKeys,
                        "The number of returned results is not equal to maxKeys" );
            }
            String NextContinuationToken = result.getNextContinuationToken();
            req.setContinuationToken( NextContinuationToken );
        } while ( result.isTruncated() );
        Assert.assertEquals( queryTime, 5, "the query time is wrong!" );
        ObjectUtils.checkListObjectsV2KeyName( objectSummaries, keyNameList );

        // startAfter match the last record
        maxKeys = 5;
        ListObjectsV2Request req2 = new ListObjectsV2Request()
                .withBucketName( bucketName )
                .withStartAfter( "%%dir" + ( objectTotalNum - 1 ) + "_16436" )
                .withMaxKeys( maxKeys );
        ListObjectsV2Result result2 = s3Client.listObjectsV2( req2 );
        List< S3ObjectSummary > objectSummaries2 = result2.getObjectSummaries();
        Assert.assertEquals( objectSummaries2.size(), 0,
                "The number of returned results is wrong" );

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
