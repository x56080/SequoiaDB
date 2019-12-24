package com.sequoias3.delimiter;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.ListVersionsRequest;
import com.amazonaws.services.s3.model.S3VersionSummary;
import com.amazonaws.services.s3.model.VersionListing;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.DelimiterUtils;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/**
 * test content: 禁用版本控制存在删除标记的对象，查询对象版本列表 testlink-case: seqDB-18142
 *
 * @author wangkexin
 * @Date 2019.04.25
 * @version 1.00
 */

public class ListObjectVersionsWithDelimiter18142 extends S3TestBase {
    private String bucketName = "bucket18142";
    private String[] keyName = { "dir1?test18142_1",
            "dir1%dir2??/dir3/test18142_2", "dir1?test18142_3",
            "dir1/dir2?aa?test18142_4", "dir1/dir2/?aa?cc?test18142_5",
            "dir1#dir2%?aa?dd?test18142_6", "dir18142", "testdir18142.txt" };
    private String hasDeleteMarkerKey = keyName[ 2 ];
    private String delimiter = "?";
    private AmazonS3 s3Client = null;
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {
        s3Client = CommLib.buildS3Client();
        // create bucket and set bucket version status
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );

        s3Client.putObject( bucketName, hasDeleteMarkerKey,
                "object_file18142" );
        // 使桶中存在versionId不为null的删除标记
        s3Client.deleteObject( bucketName, hasDeleteMarkerKey );

        CommLib.setBucketVersioning( s3Client, bucketName, "Suspended" );
        // put multiple objects
        for ( int i = 0; i < keyName.length; i++ ) {
            s3Client.putObject( bucketName, keyName[ i ], "object_file18142" );
        }

        // 使桶中存在versionId为null的删除标记
        s3Client.deleteObject( bucketName, hasDeleteMarkerKey );
        DelimiterUtils.putBucketDelimiter( bucketName, delimiter );
    }

    @Test
    public void testGetObjectList() throws Exception {
        VersionListing versionList = s3Client.listVersions(
                new ListVersionsRequest().withBucketName( bucketName )
                        .withDelimiter( delimiter ) );
        List<String> commonPrefixes = versionList.getCommonPrefixes();
        List<String> expCommPrefixes = ObjectUtils
                .getCommPrefixes( keyName, "", delimiter );
        ObjectUtils.checkListObjectsV2Commprefixes( commonPrefixes,
                expCommPrefixes );

        List<String> actVersionList = new ArrayList<>();
        List<S3VersionSummary> verList = versionList.getVersionSummaries();
        for ( S3VersionSummary version : verList ) {
            actVersionList.add( version.getKey() );
            Assert.assertEquals( version.getVersionId(), "null" );
            Assert.assertFalse( version.isDeleteMarker(),
                    "isDeleteMarker is wrong , key = " + version.getKey() );
        }
        List<String> expVersionList = ObjectUtils
                .getKeys( keyName, "", delimiter );
        Collections.sort( expVersionList );
        Assert.assertEquals( actVersionList, expVersionList );
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
