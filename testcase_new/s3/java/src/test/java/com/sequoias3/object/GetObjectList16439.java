package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.ListObjectsV2Request;
import com.amazonaws.services.s3.model.ListObjectsV2Result;
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
 * test content: 多次查询结果在commprefix中有相同记录 testlink-case: seqDB-16439
 *
 * @author wangkexin
 * @Date 2018.11.22
 * @version 1.00
 */
public class GetObjectList16439 extends S3TestBase {
    private String bucketName = "bucket16439";
    private String keyName = "/dir";
    private String prefix = "/";
    private String delimiter = "/";
    private int maxKeys = 2;
    private List< String > exCommprefixList = new ArrayList< String >();
    private int samePrefixObjNum = 5;
    private int sameDirNum = 3;
    private AmazonS3 s3Client = null;
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {
        s3Client = CommLib.buildS3Client();
        // create bucket
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );
        String currentKeyName = null;
        String expCommprefix = null;
        // put multiple objects
        for ( int i = 0; i < samePrefixObjNum; i++ ) {
            for ( int j = 0; j < sameDirNum; j++ ) {
                currentKeyName = keyName + i + "/subdir" + j + "/16439";
                s3Client.putObject( bucketName, currentKeyName,
                        "object_file16439" );
            }
            expCommprefix = keyName + i + "/";
            exCommprefixList.add( expCommprefix );
        }
    }

    @Test
    public void testGetObjectList() throws Exception {
        ListObjectsV2Request req = new ListObjectsV2Request()
                .withBucketName( bucketName ).withPrefix( prefix )
                .withDelimiter( delimiter ).withMaxKeys( maxKeys );
        ListObjectsV2Result result;
        List< String > commprefixesResult = new ArrayList<>();
        // currentTurn is query times
        int queryTime = 0;

        do {
            queryTime++;
            result = s3Client.listObjectsV2( req );
            commprefixesResult.addAll( result.getCommonPrefixes() );
            if ( queryTime == 3 ) {
                Assert.assertEquals( result.getKeyCount(), 1,
                        "The expected results do not match the actual number of returns" );
                Assert.assertFalse( result.isTruncated(),
                        "last turn result.isTruncated should be false!" );
            } else {
                Assert.assertEquals( result.getKeyCount(), maxKeys,
                        "The expected results do not match the actual number of returns" );
                Assert.assertTrue( result.isTruncated(),
                        "when it is not last turn result.isTruncated should be true!" );
            }
            String NextContinuationToken = result.getNextContinuationToken();
            req.setContinuationToken( NextContinuationToken );

        } while ( result.isTruncated() );
        Assert.assertEquals( queryTime, 3, "the query time is wrong!" );
        ObjectUtils.checkListObjectsV2Commprefixes( commprefixesResult,
                exCommprefixList );
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
