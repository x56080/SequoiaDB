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
 * test content: 带分隔符查询桶中对象版本列表，匹配部分记录 testlink-case: seqDB-18139
 *
 * @author wangkexin
 * @Date 2019.04.24
 * @version 1.00
 */

public class ListObjectVersionsWithDelimiter18139 extends S3TestBase {
    private String bucketName = "bucket18139";
    private String keyName = "dir";
    private String delimiter = "?";
    private String repeatedKeyName1 = "dir1?test18139";
    private String repeatedKeyName2 = "dir1test18139";
    private int versionNum = 3;
    private List<String> keyList = new ArrayList<String>();
    private List<String> versions = new ArrayList<String>();
    private int objectWithDelimiterNum = 150;
    private int objectWithoutDelimiterNum = 50;
    private AmazonS3 s3Client = null;
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {
        s3Client = CommLib.buildS3Client();
        // create bucket and set bucket version status
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );

        // put objects with delimiter
        for ( int i = 0; i < objectWithDelimiterNum; i++ ) {
            String currentKeyName = keyName + i + delimiter + "test18139";
            s3Client.putObject( bucketName, currentKeyName,
                    "object_file18139" );
            keyList.add( currentKeyName );
        }

        // put objects without delimiter
        for ( int i = 0; i < objectWithoutDelimiterNum; i++ ) {
            String currentKeyName = keyName + i + "test18139";
            s3Client.putObject( bucketName, currentKeyName,
                    "object_file18139" );
            versions.add( currentKeyName );
        }

        // 使桶中存在包含分隔符的对象和不包含分隔符的对象都有多个版本(versionNum)的情况
        for ( int i = 0; i < versionNum - 1; i++ ) {
            s3Client.putObject( bucketName, repeatedKeyName1,
                    "object_file18139" );
            s3Client.putObject( bucketName, repeatedKeyName2,
                    "object_file18139" );
            versions.add( repeatedKeyName2 );
        }
        DelimiterUtils.putBucketDelimiter( bucketName, delimiter );
    }

    @Test
    public void testGetObjectList() throws Exception {
        // db端查看访问计划显示索引为目录表索引
        VersionListing versionList = s3Client.listVersions(
                new ListVersionsRequest().withBucketName( bucketName )
                        .withDelimiter( delimiter ) );
        List<String> commonPrefixes = versionList.getCommonPrefixes();

        String[] objectNames = new String[ keyList.size() ];
        List<String> expresultList = ObjectUtils
                .getCommPrefixes( keyList.toArray( objectNames ), "",
                        delimiter );
        ObjectUtils.checkListObjectsV2Commprefixes( commonPrefixes,
                expresultList );

        List<String> actVersionsKeyName = new ArrayList<String>();
        List<S3VersionSummary> versionLists = versionList.getVersionSummaries();
        List<String> versionId = new ArrayList<>();
        for ( S3VersionSummary s3VersionSummary : versionLists ) {
            actVersionsKeyName.add( s3VersionSummary.getKey() );
            if ( s3VersionSummary.getKey().equals( repeatedKeyName2 ) ) {
                versionId.add( s3VersionSummary.getVersionId() );
            } else {
                Assert.assertEquals( s3VersionSummary.getVersionId(), "0" );
            }
        }

        Collections.sort( versions );
        Assert.assertEquals( actVersionsKeyName, versions,
                "the returned result by versions is wrong, act: "
                        + actVersionsKeyName.toString() + ", exp: " + versions
                        .toString() );

        List<String> expVersionIdList = getVersionIdList();
        Assert.assertEquals( versionId, expVersionIdList );
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

    private List<String> getVersionIdList() {
        List<String> versionIdList = new ArrayList<>();
        for ( int i = versionNum - 1; i > -1; i-- ) {
            versionIdList.add( String.valueOf( i ) );
        }
        return versionIdList;
    }
}
