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
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * test content: 带分隔符delimiter和maxkeys查询 testlink-case: seqDB-16393
 *
 * @author wangkexin
 * @Date 2018.11.27
 * @version 1.00
 */
public class GetObjectVersionList16393 extends S3TestBase {
    private String bucketName = "bucket16393";
    private String[] keyName = { "dir1/test1", "dir2/dir2_1/test2",
            "dir3/dir3_1/test3", "test4", "test5" };
    private String delimiter = "/";
    private List< String > expCommonPrefixes = new ArrayList< String >();
    private List< String > expVersionsKeyName = new ArrayList< String >();
    private String content = "object16393";
    private AmazonS3 s3Client = null;
    private AtomicInteger actSuccessTests = new AtomicInteger( 0 );

    @DataProvider(name = "maxKeyProvider")
    public Object[][] generateRemoveIndex() {
        return new Object[][] {
                // delimiter满足对象记录数（(commprefixes:3) + (versions:2*2) =
                // 7）大于maxKeys
                new Object[] { 1, 7 },
                // delimiter满足对象记录数（(commprefixes:3) + (versions:2*2) =
                // 7）小于maxKeys
                new Object[] { 8, 1 } };
    }

    @BeforeClass
    private void setUp() throws Exception {
        s3Client = CommLib.buildS3Client();
        // create bucket
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );
        for ( int i = 0; i < keyName.length; i++ ) {
            // 获取对象匹配前缀的记录：dir1/、dir2/dir2_1/、dir3/dir3_1/
            if ( i < 3 ) {
                expCommonPrefixes.add( keyName[ i ].substring( 0,
                        keyName[ i ].indexOf( delimiter ) + 1 ) );
            }
            s3Client.putObject( bucketName, keyName[ i ], content );
            s3Client.putObject( bucketName, keyName[ i ], content );
        }
        // Versions 中预期key 为"test4", "test5"
        expVersionsKeyName.add( keyName[ 3 ] );
        expVersionsKeyName.add( keyName[ 3 ] );
        expVersionsKeyName.add( keyName[ 4 ] );
        expVersionsKeyName.add( keyName[ 4 ] );

        Collections.sort( expCommonPrefixes );
        Collections.sort( expVersionsKeyName );
    }

    @Test(dataProvider = "maxKeyProvider")
    public void testGetObjectList( int maxKeys, int expQueryTimes )
            throws Exception {
        List< String > actCommonPrefixes = new ArrayList< String >();
        List< String > actVersionsKeyName = new ArrayList< String >();
        int queryTimes = 0;
        ListVersionsRequest req = new ListVersionsRequest()
                .withBucketName( bucketName ).withDelimiter( delimiter )
                .withMaxResults( maxKeys );
        VersionListing versionList = s3Client.listVersions( req );
        while ( true ) {
            List< String > commprefixesResult = versionList.getCommonPrefixes();
            for ( String s : commprefixesResult ) {
                actCommonPrefixes.add( s );
            }
            List< S3VersionSummary > verList = versionList
                    .getVersionSummaries();
            for ( S3VersionSummary s3VersionSummary : verList ) {
                actVersionsKeyName.add( s3VersionSummary.getKey() );
            }

            queryTimes++;
            if ( versionList.isTruncated() ) {
                versionList = s3Client.listNextBatchOfVersions( versionList );
            } else {
                break;
            }
        }

        // check result
        Assert.assertEquals( queryTimes, expQueryTimes,
                "The total number of results is incorrect" );
        Assert.assertEquals( actCommonPrefixes, expCommonPrefixes,
                "the number of results returned by commonPrefixes is wrong, act: "
                        + actCommonPrefixes.toString() + ", exp: "
                        + expCommonPrefixes.toString() );
        Assert.assertEquals( actVersionsKeyName, expVersionsKeyName,
                "the number of results returned by versions is wrong, act: "
                        + actVersionsKeyName.toString() + ", exp: "
                        + expVersionsKeyName.toString() );

        actSuccessTests.getAndIncrement();
    }

    @AfterClass
    private void tearDown() {
        if ( actSuccessTests.get() == generateRemoveIndex().length ) {
            CommLib.deleteAllObjectVersions( s3Client, bucketName );
            s3Client.deleteBucket( bucketName );
        }
    }
}
