package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.ListObjectsV2Request;
import com.amazonaws.services.s3.model.ListObjectsV2Result;
import com.amazonaws.services.s3.model.S3ObjectSummary;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/**
 * @Description: 指定nextContinuationToken匹配记录被删除，查询列表元数据
 *
 * @author wangkexin
 * @Date 2018.11.16
 * @version 1.00
 */
public class GetObjectList16437 extends S3TestBase {
    private String bucketName = "bucket16437";
    private String keyName = "&dir&dir";
    private List< String > expresultList = new ArrayList< String >( 10 );
    private int objectTotalNum = 10;
    private AmazonS3 s3Client = null;
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {
        s3Client = CommLib.buildS3Client();
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );

        // put multiple objects
        for ( int i = 0; i < objectTotalNum; i++ ) {
            String currentKeyName = keyName + i + "_16437";
            s3Client.putObject( bucketName, currentKeyName,
                    "object_file16437" );
            expresultList.add( currentKeyName );
        }
    }

    @Test
    public void testGetObjectList() throws Exception {
        int keyCount = 2;
        // first query
        ListObjectsV2Request req = new ListObjectsV2Request()
                .withBucketName( bucketName ).withMaxKeys( keyCount );
        ListObjectsV2Result result = s3Client.listObjectsV2( req );
        List< S3ObjectSummary > objectSummaries = result.getObjectSummaries();
        checkListObjectsV2Result( objectSummaries, 0, keyCount );
        String NextContinuationToken = result.getNextContinuationToken();

        // delete object that next token points to
        s3Client.deleteObject( bucketName, expresultList.get( keyCount ) );
        expresultList.remove( keyCount );

        // second query
        ListObjectsV2Request req2 = new ListObjectsV2Request()
                .withBucketName( bucketName )
                .withContinuationToken( NextContinuationToken );
        ListObjectsV2Result result2 = s3Client.listObjectsV2( req2 );
        List< S3ObjectSummary > objectSummaries2 = result2.getObjectSummaries();
        checkListObjectsV2Result( objectSummaries2, keyCount,
                objectTotalNum - keyCount - 1 );

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        if ( runSuccess ) {
            CommLib.deleteAllObjectVersions( s3Client, bucketName );
            s3Client.deleteBucket( bucketName );
        }
    }

    private void checkListObjectsV2Result(
            List< S3ObjectSummary > objectSummaries, int startIndex,
            int expCount ) {
        Assert.assertEquals( objectSummaries.size(), expCount,
                "The number of returned results is wrong" );
        Collections.sort( expresultList );
        for ( int i = 0; i < objectSummaries.size(); i++ ) {
            Assert.assertEquals( objectSummaries.get( i ).getKey(),
                    expresultList.get( startIndex ), "contents is wrong" );
            startIndex++;
        }
    }
}
