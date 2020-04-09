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

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/**
 * test content: 带分隔符delimiter查询对象版本列表 testlink-case: seqDB-16392
 *
 * @author wangkexin
 * @Date 2018.11.24
 * @version 1.00
 */
public class GetObjectVersionList16392 extends S3TestBase {
    private String bucketName = "bucket16392";
    private String[] keyName = { "dir1/test1", "dir1/dir2/test2", "test3",
            "test4" };
    private String delimiter = "/";
    private String expPrefix = "dir1/";
    private List< String > expVersionsKeyName = new ArrayList< String >();
    private List< String > expVersionsKeyEtag = new ArrayList< String >();
    private String[] expVersionId = { "1", "0", "1", "0" };
    private String content = "object16392";
    private AmazonS3 s3Client = null;
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {
        s3Client = CommLib.buildS3Client();
        // create bucket
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );

        for ( int i = 0; i < keyName.length; i++ ) {
            List< String > tempEtag = new ArrayList<>();
            String currentContent = content + ObjectUtils.getRandomString( i );
            s3Client.putObject( bucketName, keyName[ i ], currentContent );
            tempEtag.add( TestTools.getMD5( currentContent.getBytes() ) );

            currentContent = content + ObjectUtils.getRandomString( i );
            s3Client.putObject( bucketName, keyName[ i ], currentContent );
            tempEtag.add( TestTools.getMD5( currentContent.getBytes() ) );
            // 同一个对象不同版本的etag，最新版本排在最前面
            Collections.reverse( tempEtag );
            expVersionsKeyEtag.addAll( tempEtag );
        }

        // verions 里面存放匹配不到delimiter的 "test3"、"test4"
        expVersionsKeyName.add( keyName[ 2 ] );
        expVersionsKeyName.add( keyName[ 2 ] );
        expVersionsKeyName.add( keyName[ 3 ] );
        expVersionsKeyName.add( keyName[ 3 ] );
        // 将"dir1/test1","dir1/dir2/test2" 的etag从expVersionsKeyEtag里面去掉
        for ( int i = 0; i < 4; i++ ) {
            expVersionsKeyEtag.remove( 0 );
        }
    }

    @Test
    public void testGetObjectList() throws Exception {
        VersionListing versionList = s3Client.listVersions(
                new ListVersionsRequest().withBucketName( bucketName )
                        .withDelimiter( delimiter ) );
        Assert.assertEquals( versionList.getCommonPrefixes().size(), 1,
                "the number of results returned by commonPrefixes is wrong" );
        Assert.assertEquals( versionList.getCommonPrefixes().get( 0 ),
                expPrefix, "the result of commonPrefixes is wrong" );
        List< S3VersionSummary > verList = versionList.getVersionSummaries();

        checkVersionsResult( verList );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        if ( runSuccess ) {
            CommLib.deleteAllObjectVersions( s3Client, bucketName );
            s3Client.deleteBucket( bucketName );
        }
    }

    private void checkVersionsResult( List< S3VersionSummary > verList ) {
        Assert.assertEquals( verList.size(), expVersionsKeyName.size(),
                "The number of results returned does not match the expected value" );
        for ( int i = 0; i < verList.size(); i++ ) {
            Assert.assertEquals( verList.get( i ).getKey(),
                    expVersionsKeyName.get( i ),
                    "the result of versions is wrong!" );
            Assert.assertEquals( verList.get( i ).getVersionId(),
                    expVersionId[ i ], "version id is wrong! the key is : "
                            + verList.get( i ).getKey() );
            Assert.assertEquals( verList.get( i ).getETag(),
                    expVersionsKeyEtag.get( i ), "etag is wrong! the key is : "
                            + verList.get( i ).getKey() );
        }
    }
}
