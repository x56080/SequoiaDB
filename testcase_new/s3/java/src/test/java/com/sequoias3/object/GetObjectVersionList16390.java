package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.ListVersionsRequest;
import com.amazonaws.services.s3.model.S3VersionSummary;
import com.amazonaws.services.s3.model.VersionListing;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.text.ParseException;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;

/**
 * test content: 带前缀prefix查询对象版本列表 testlink-case: seqDB-16390
 *
 * @author wangkexin
 * @Date 2018.11.23
 * @version 1.00
 */
public class GetObjectVersionList16390 extends S3TestBase {
    private String bucketName = "bucket16390";
    private String[] keyName = { "dir1%dir2%test1_16390",
            "dir1%dir2%test2_16390", "test3_16390", "test4_16390" };
    private String prefix = "dir1";
    private List<String> expEtagList = new ArrayList<String>();
    private String content = "object16390";
    private List<long[]> dateRangeList = new ArrayList<>();
    private AmazonS3 s3Client = null;
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {
        s3Client = CommLib.buildS3Client();
        // create bucket
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );

        for ( int i = 0; i < keyName.length; i++ ) {
            long dateRange[] = new long[ 2 ];
            dateRange[ 0 ] = System.currentTimeMillis();

            String currentContent = content + ObjectUtils.getRandomString( i );
            s3Client.putObject( bucketName, keyName[ i ], currentContent );

            dateRange[ 1 ] = System.currentTimeMillis();
            expEtagList.add( TestTools.getMD5( currentContent.getBytes() ) );
            dateRangeList.add( dateRange );
        }
    }

    @Test
    public void testGetObjectList() throws Exception {
        VersionListing versionList = s3Client.listVersions(
                new ListVersionsRequest().withBucketName( bucketName )
                        .withPrefix( prefix ) );
        List<S3VersionSummary> verList = versionList.getVersionSummaries();
        checklistVersionsResult( verList );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        if ( runSuccess ) {
            CommLib.deleteAllObjectVersions( s3Client, bucketName );
            s3Client.deleteBucket( bucketName );
        }
    }

    private void checklistVersionsResult( List<S3VersionSummary> versions )
            throws ParseException {
        Assert.assertEquals( versions.size(), 2,
                "The number of results returned does not match the expected value" );
        for ( int i = 0; i < 2; i++ ) {
            Assert.assertEquals( versions.get( i ).getKey(), keyName[ i ],
                    "versions' key is wrong" );
            Assert.assertEquals( versions.get( i ).getVersionId(), "null",
                    "versions' versionid is wrong" );
            Assert.assertEquals( versions.get( i ).getSize(),
                    ( long ) ( content.length() + i ),
                    "versions' size is wrong" );
            Assert.assertEquals( versions.get( i ).getETag(),
                    expEtagList.get( i ), "versions' Etag is wrong" );

            // 校验对象lastModified时间在[date1, date2]范围内，因时区问题再加8小时
            Date actDate = versions.get( i ).getLastModified();

            long actDateTime = actDate.getTime();
            long date1Time = dateRangeList.get( i )[ 0 ] + 28800000L;
            long date2Time = dateRangeList.get( i )[ 1 ] + 28800000L;

            if ( actDateTime < date1Time || actDateTime > date2Time ) {
                Assert.fail( "lastmodified is wrong!  actDate is : " + actDate
                        .getTime() + ", date1 is :" + dateRangeList
                        .get( i )[ 0 ] + ", date2 is : " + dateRangeList
                        .get( i )[ 1 ] );
            }
        }
    }
}
