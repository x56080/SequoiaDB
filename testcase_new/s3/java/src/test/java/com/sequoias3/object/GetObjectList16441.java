package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.ListObjectsV2Request;
import com.amazonaws.services.s3.model.ListObjectsV2Result;
import com.amazonaws.services.s3.model.S3ObjectSummary;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import com.sequoias3.testcommon.s3utils.UserUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.ArrayList;
import java.util.List;

/**
 * test content: 带指定fetch-owner查询对象元数据列表，显示所有者信息 testlink-case: seqDB-16441
 *
 * @author wangkexin
 * @Date 2018.11.23
 * @version 1.00
 */
public class GetObjectList16441 extends S3TestBase {
    private String bucketName = "bucket16441";
    private String userName = "user16441";
    private String keyName = "/dir/dir";
    private String prefix = "/dir/";
    private String delimiter = "/";
    private List< String > expCommPrefixesList = new ArrayList< String >();
    private List< String > expContentKeyList = new ArrayList< String >();
    private int commPrefixesNums = 10;
    private int contentNums = 5;
    private String[] accessKeys = null;
    private AmazonS3 s3Client = null;
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {
        accessKeys = UserUtils.createUser( userName, "normal" );
        s3Client = CommLib.buildS3Client( accessKeys[ 0 ], accessKeys[ 1 ] );
        // create bucket
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );

        // put multiple objects
        for ( int i = 0; i < commPrefixesNums; i++ ) {
            String currentKeyName = keyName + i + "/16441";
            s3Client.putObject( bucketName, currentKeyName,
                    "object_file16441" );
            expCommPrefixesList.add( currentKeyName.substring( 0,
                    currentKeyName.lastIndexOf( delimiter ) + 1 ) );
        }

        // put another objects
        for ( int i = 0; i < contentNums; i++ ) {
            String currentKeyName = prefix + "16441_" + i;
            s3Client.putObject( bucketName, currentKeyName,
                    "object_file16441" );
            expContentKeyList.add( currentKeyName );
        }
    }

    @Test
    public void testGetObjectList() throws Exception {
        ListObjectsV2Request req = new ListObjectsV2Request()
                .withBucketName( bucketName ).withPrefix( prefix )
                .withDelimiter( delimiter ).withFetchOwner( true );
        ListObjectsV2Result result = s3Client.listObjectsV2( req );
        List< String > commprefixesResult = result.getCommonPrefixes();
        ObjectUtils.checkListObjectsV2Commprefixes( commprefixesResult,
                expCommPrefixesList );

        List< S3ObjectSummary > objectSummaries = result.getObjectSummaries();
        checkContentsResult( objectSummaries );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        if ( runSuccess ) {
            CommLib.deleteAllObjectVersions( s3Client, bucketName );
            s3Client.deleteBucket( bucketName );
            UserUtils.deleteUser( userName );
        }
    }

    private void checkContentsResult(
            List< S3ObjectSummary > objectSummaries ) {
        Assert.assertEquals( objectSummaries.size(), expContentKeyList.size() );
        for ( int i = 0; i < objectSummaries.size(); i++ ) {
            Assert.assertEquals( objectSummaries.get( i ).getKey(),
                    expContentKeyList.get( i ) );
            Assert.assertEquals(
                    objectSummaries.get( i ).getOwner().getDisplayName(),
                    userName );
        }
    }
}
