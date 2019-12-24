package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.ListObjectsV2Request;
import com.amazonaws.services.s3.model.ListObjectsV2Result;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.List;

/**
 * test content: 带prefix、start-after、delimiter匹配查询对象元数据列表 testlink-case:
 * seqDB-16432
 *
 * @author wangkexin
 * @Date 2018.11.15
 * @version 1.00
 */
public class GetObjectList16432 extends S3TestBase {
    private String bucketName = "bucket16432";
    private String keyName1 = "/dir/dir1/dir2/object16432_a";
    private String keyName2 = "/dir/dir1/dir2/object16432_b";
    private String keyName3 = "/dir1/dir2/object16432_1";
    private String keyName4 = "/dir1/dir2/object16432_2";
    private String keyName5 = "/dir1/object16432_3";
    private String keyName6 = "object16432_4";
    private String prefix = "/dir1/";
    private String delimiter = "/";
    private String expresult = "/dir1/dir2/";
    private AmazonS3 s3Client = null;
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {
        s3Client = CommLib.buildS3Client();
        // create bucket and set bucket version status
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );

        // put multiple objects
        s3Client.putObject( bucketName, keyName1, "object1_file16432" );
        s3Client.putObject( bucketName, keyName2, "object2_file16432" );
        s3Client.putObject( bucketName, keyName3, "object3_file16432" );
        s3Client.putObject( bucketName, keyName4, "object4_file16432" );
        s3Client.putObject( bucketName, keyName5, "object5_file16432" );
        s3Client.putObject( bucketName, keyName6, "object6_file16432" );
    }

    @Test
    public void testGetObjectList() throws Exception {
        ListObjectsV2Request req = new ListObjectsV2Request()
                .withBucketName( bucketName ).withPrefix( prefix )
                .withDelimiter( delimiter ).withStartAfter( keyName1 );
        ListObjectsV2Result result = s3Client.listObjectsV2( req );
        List<String> commprefixesResult = result.getCommonPrefixes();

        // check result
        checkListObjectsV2Result( commprefixesResult );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        if ( runSuccess ) {
            CommLib.deleteAllObjectVersions( s3Client, bucketName );
            s3Client.deleteBucket( bucketName );
        }
    }

    private void checkListObjectsV2Result( List<String> resultList ) {
        Assert.assertEquals( resultList.size(), 1,
                "The expected results do not match the actual number of returns" );
        Assert.assertEquals( resultList.get( 0 ), expresult,
                "commonPrefixes is wrong" );
    }
}
