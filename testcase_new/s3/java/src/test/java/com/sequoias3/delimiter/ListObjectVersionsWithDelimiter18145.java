package com.sequoias3.delimiter;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.ListVersionsRequest;
import com.amazonaws.services.s3.model.VersionListing;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.DelimiterUtils;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.List;

/**
 * test content: 带前缀prefix和delimiter查询对象版本列表，匹配删除标记对象 testlink-case: seqDB-18145
 *
 * @author wangkexin
 * @Date 2019.04.25
 * @version 1.00
 */

public class ListObjectVersionsWithDelimiter18145 extends S3TestBase {
    private String bucketName = "bucket18145";
    private String[] keyName = { "/aa/dd/test18145_1.txt",
            "/aa/cc/test18145_2.txt", "/aa/test18145_3" };
    private String delimiter = "t";
    private String prefix = "/aa/";
    private AmazonS3 s3Client = null;
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {
        s3Client = CommLib.buildS3Client();
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );

        // 制造删除标记对象
        for ( String objectName : keyName ) {
            s3Client.deleteObject( bucketName, objectName );
        }
        DelimiterUtils.putBucketDelimiter( bucketName, delimiter );
    }

    @Test
    public void testGetObjectList() throws Exception {
        VersionListing versionList = s3Client.listVersions(
                new ListVersionsRequest().withBucketName( bucketName )
                        .withDelimiter( delimiter ).withPrefix( prefix ) );
        List<String> commonPrefixes = versionList.getCommonPrefixes();
        List<String> expCommPrefixes = ObjectUtils
                .getCommPrefixes( keyName, "", delimiter );
        ObjectUtils.checkListObjectsV2Commprefixes( commonPrefixes,
                expCommPrefixes );
        Assert.assertEquals( versionList.getVersionSummaries().size(), 0 );

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
