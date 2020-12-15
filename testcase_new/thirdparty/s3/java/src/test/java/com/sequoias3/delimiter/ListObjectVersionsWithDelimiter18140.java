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
 * @Description: 不开启版本控制，带分隔符查询桶中对象版本列表 testlink-case: seqDB-18140
 *
 * @author wangkexin
 * @Date 2019.04.25
 * @version 1.00
 */

public class ListObjectVersionsWithDelimiter18140 extends S3TestBase {
    private String bucketName = "bucket18140";
    private String keyName = "dir";
    private String delimiter = "?";
    private String sameDirKeyName[] = { "dir1?test123", "dir10?test123",
            "dir50?test123" };
    private String[] objectNames = new String[ 200 ];
    private AmazonS3 s3Client = null;
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {
        s3Client = CommLib.buildS3Client();
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );

        // put multiple objects
        for ( int i = 0; i < objectNames.length; i++ ) {
            String currentKeyName = keyName + i + delimiter + "test18140";
            s3Client.putObject( bucketName, currentKeyName,
                    "object_file18140" );
            objectNames[ i ] = currentKeyName;
        }

        // put object same directory object
        for ( int i = 0; i < sameDirKeyName.length; i++ ) {
            s3Client.putObject( bucketName, sameDirKeyName[ i ],
                    "object_file18140" );
        }
        DelimiterUtils.putBucketDelimiter( bucketName, delimiter );
    }

    @Test
    public void testGetObjectList() throws Exception {
        // db端查看访问计划显示索引为目录表索引
        VersionListing versionList = s3Client.listVersions(
                new ListVersionsRequest().withBucketName( bucketName )
                        .withDelimiter( delimiter ) );
        List< String > commonPrefixes = versionList.getCommonPrefixes();

        List< String > expCommprefixes = ObjectUtils
                .getCommPrefixes( objectNames, "", delimiter );
        ObjectUtils.checkListObjectsV2Commprefixes( commonPrefixes,
                expCommprefixes );
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
