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
import org.springframework.util.LinkedMultiValueMap;
import org.springframework.util.MultiValueMap;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;

/**
 * test content: 带分隔符delimiter和maxkeys查询 testlink-case: seqDB-18148
 *
 * @author wangkexin
 * @Date 2019.04.28
 * @version 1.00
 */

public class ListObjectVersionsWithDelimiter18148 extends S3TestBase {
    private String bucketName = "bucket18148";
    private String[] keyNames = { "dir1/test18148_1", "dir1/test18148_1",
            "dir1/test18148_1", "dir1/dir2/test18148_2",
            "dir1/dir2/test18148_2", "dir1/dir2/test18148_2",
            "/dir/test/test18148_3", "test18148_4", "dir1/test1/test18148_5",
            "dir/log", "dir/log", "dir/log" };

    private String versionsKey = "dir/log";
    private List<String> expCommPrefixes = new ArrayList<>();
    private MultiValueMap<String, String> expVersionsMap = new LinkedMultiValueMap<String, String>();
    private String delimiter = "tes";
    private int versionNum = 3;
    private AmazonS3 s3Client = null;
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {
        s3Client = CommLib.buildS3Client();
        // create bucket and set bucket version status
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );

        // put multiple objects
        for ( String objectName : keyNames ) {
            s3Client.putObject( bucketName, objectName, "object_file18148" );
        }

        DelimiterUtils.putBucketDelimiter( bucketName, delimiter );

        expCommPrefixes = ObjectUtils
                .getCommPrefixes( keyNames, "", delimiter );
        for ( int i = versionNum - 1; i >= 0; i-- ) {
            expVersionsMap.add( versionsKey, String.valueOf( i ) );
        }
    }

    @Test
    public void testGetObjectList() throws Exception {
        // maxKeys 大于匹配记录数时一次返回对象版本列表的所有匹配结果，这里设置MaxResults为10
        ListVersionsAtOnce( 10,
                expCommPrefixes.size() + expVersionsMap.get( versionsKey )
                        .size() );

        // maxKeys 小于匹配记录数时分多次返回对象版本列表的所有匹配结果，这里设置MaxResults为1
        ListVersionsWithMultTimes( 1 );

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

    private void ListVersionsAtOnce( int maxResults, int expOnceReturnedNum ) {
        ListVersionsRequest req = new ListVersionsRequest()
                .withBucketName( bucketName ).withDelimiter( delimiter )
                .withMaxResults( maxResults );

        VersionListing versionList = s3Client.listVersions( req );
        List<String> commonPrefixes = new ArrayList<String>();
        int onceReturn = 0;
        commonPrefixes.addAll( versionList.getCommonPrefixes() );
        onceReturn += versionList.getCommonPrefixes().size();
        List<S3VersionSummary> verList = versionList.getVersionSummaries();

        onceReturn += verList.size();
        Assert.assertEquals( onceReturn, expOnceReturnedNum,
                "commonPrefixes : " + commonPrefixes.toString() );
        if ( !versionList.isTruncated() ) {
            ObjectUtils.checkListVSResults( versionList, expCommPrefixes,
                    expVersionsMap );
        } else {
            Assert.fail( "vsList.isTruncated() must be false" );
        }
    }

    private void ListVersionsWithMultTimes( int maxResults ) {
        ListVersionsRequest req = new ListVersionsRequest()
                .withBucketName( bucketName ).withDelimiter( delimiter )
                .withMaxResults( maxResults );

        VersionListing versionList = s3Client.listVersions( req );
        List<String> actCommonPrefixes = new ArrayList<String>();
        MultiValueMap<String, String> actVersionsMap = new LinkedMultiValueMap<String, String>();
        while ( true ) {
            int onceReturn = 0;
            actCommonPrefixes.addAll( versionList.getCommonPrefixes() );
            onceReturn += versionList.getCommonPrefixes().size();
            List<S3VersionSummary> verList = versionList.getVersionSummaries();
            for ( S3VersionSummary versionSummary : verList ) {
                actVersionsMap.add( versionSummary.getKey(),
                        versionSummary.getVersionId() );
            }

            onceReturn += verList.size();
            Assert.assertEquals( onceReturn, maxResults,
                    "commonPrefixes : " + actCommonPrefixes.toString() );
            if ( versionList.isTruncated() ) {
                versionList = s3Client.listNextBatchOfVersions( versionList );
            } else {
                break;
            }
        }

        Assert.assertEquals( actCommonPrefixes, expCommPrefixes,
                "actCommonPrefixes = " + actCommonPrefixes.toString()
                        + ",expCommonPrefixes = " + expCommPrefixes
                        .toString() );
        Assert.assertEquals( actVersionsMap.size(), expVersionsMap.size(),
                "actMap = " + actVersionsMap.toString() + ",expVersionsMap = "
                        + expVersionsMap.toString() );
        for ( Map.Entry<String, List<String>> entry : expVersionsMap
                .entrySet() ) {
            Assert.assertEquals( actVersionsMap.get( entry.getKey() ),
                    expVersionsMap.get( entry.getKey() ),
                    "actMap = " + actVersionsMap.toString()
                            + ",expVersionsMap = " + expVersionsMap
                            .toString() );
        }
    }
}
