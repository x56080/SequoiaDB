package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.ListVersionsRequest;
import com.amazonaws.services.s3.model.S3VersionSummary;
import com.amazonaws.services.s3.model.VersionListing;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.List;

/**
 * @Description: 带前缀prefix查询对象版本列表，匹配不到对象数据 testlink-case: seqDB-16391
 *
 * @author wangkexin
 * @Date 2018.11.24
 * @version 1.00
 */
public class GetObjectVersionList16391 extends S3TestBase {
    private String bucketName = "bucket16391";
    private String[] keyName = { "dir1%test1", "dir1%dir2%test2", "test3",
            "test4" };
    private String prefix = "dir2";
    private String file = "object16391";
    private AmazonS3 s3Client = null;
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {
        s3Client = CommLib.buildS3Client();
        // create bucket
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );
        for ( int i = 0; i < keyName.length; i++ ) {
            s3Client.putObject( bucketName, keyName[ i ], file );
        }
    }

    @Test
    public void testGetObjectList() throws Exception {
        VersionListing versionList = s3Client
                .listVersions( new ListVersionsRequest()
                        .withBucketName( bucketName ).withPrefix( prefix ) );
        List< S3VersionSummary > verList = versionList.getVersionSummaries();
        Assert.assertEquals( verList.size(), 0, "act versionList is not null" );
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
