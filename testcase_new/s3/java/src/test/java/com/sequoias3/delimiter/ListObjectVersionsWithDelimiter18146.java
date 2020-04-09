package com.sequoias3.delimiter;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.ListVersionsRequest;
import com.amazonaws.services.s3.model.S3VersionSummary;
import com.amazonaws.services.s3.model.VersionListing;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.DelimiterUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.List;

/**
 * test content: 带前缀prefix和delimiter查询对象版本列表，不匹配prefix testlink-case:
 * seqDB-18146
 *
 * @author wangkexin
 * @Date 2019.04.28
 * @version 1.00
 */

public class ListObjectVersionsWithDelimiter18146 extends S3TestBase {
    private String bucketName = "bucket18146";
    private String[] keyName = { "/dir1/dir2/test18146_1.txt",
            "/dir1/dir2/test18146_2.txt", "test18146_3", "test18146_4" };
    private String delimiter = "/";
    private String prefix = "dir2";
    private AmazonS3 s3Client = null;
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {
        s3Client = CommLib.buildS3Client();
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );

        for ( String objectName : keyName ) {
            s3Client.putObject( bucketName, objectName, "object_file18146" );
        }
    }

    @Test
    public void testGetObjectList() throws Exception {
        DelimiterUtils.checkCurrentDelimiteInfo( bucketName, delimiter );

        VersionListing versionList = s3Client.listVersions(
                new ListVersionsRequest().withBucketName( bucketName )
                        .withDelimiter( delimiter ).withPrefix( prefix ) );
        List< String > commonPrefixes = versionList.getCommonPrefixes();
        Assert.assertEquals( commonPrefixes.size(), 0 );

        List< S3VersionSummary > s3VersionSummaries = versionList
                .getVersionSummaries();
        Assert.assertEquals( s3VersionSummaries.size(), 0 );

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
