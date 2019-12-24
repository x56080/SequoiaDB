package com.sequoias3.object.concurrent;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.ListVersionsRequest;
import com.amazonaws.services.s3.model.S3VersionSummary;
import com.amazonaws.services.s3.model.VersionListing;
import com.sequoiadb.exception.BaseException;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.S3ThreadBase;
import com.sequoias3.testcommon.s3utils.UserUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/**
 * test content: 开启版本控制，并发不同条件查询对象版本列表 testlink-case: seqDB-16499
 *
 * @author wangkexin
 * @Date 2019.01.03
 * @version 1.00
 */
public class GetObjectListWithVersion16499 extends S3TestBase {
    private String bucketName = "bucket16499";
    private String userName = "user16499";
    private String roleName = "normal";
    private String keyName = "/dir/dir";
    private String prefix = "/dir";
    private String delimiter = "/";
    private List<String> expresultList1 = new ArrayList<String>();
    private List<String> expresultList2 = new ArrayList<String>();
    private List<String> expresultList3 = new ArrayList<String>();
    private int objectTotalNum = 50;
    private AmazonS3 s3Client = null;
    private String[] acessKeys = null;
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {
        CommLib.clearUser( userName );
        acessKeys = UserUtils.createUser( userName, roleName );
        s3Client = CommLib.buildS3Client( acessKeys[ 0 ], acessKeys[ 1 ] );
        // create bucket and set bucket version status
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );

        // put multiple objects
        for ( int i = 0; i < objectTotalNum; i++ ) {
            String currentKeyName = keyName + i + "/16499";
            s3Client.putObject( bucketName, currentKeyName,
                    "object_file16499" );
            expresultList1.add( currentKeyName );
            expresultList2.add( currentKeyName );
        }

        // put another objects that do not match prefix
        s3Client.putObject( bucketName, "/testa16499", "object_file16499" );
        s3Client.putObject( bucketName, "/testb16499", "object_file16499" );
        expresultList1.add( "/testa16499" );
        expresultList1.add( "/testb16499" );

        Collections.sort( expresultList1 );
        Collections.sort( expresultList2 );
        expresultList3.add( "/dir/" );
    }

    @Test
    public void testGetObjectList() throws Exception {
        ListVersionThread listVersion = new ListVersionThread();
        ListVersionWithPerfixThread listVersionWithPerfix = new ListVersionWithPerfixThread();
        ListVersionWithPerfixAndDelimiterThread listVersionWithPerfixAndDelimiter = new ListVersionWithPerfixAndDelimiterThread();
        listVersion.start();
        listVersionWithPerfix.start();
        listVersionWithPerfixAndDelimiter.start();

        Assert.assertTrue( listVersion.isSuccess(), listVersion.getErrorMsg() );
        Assert.assertTrue( listVersionWithPerfix.isSuccess(),
                listVersionWithPerfix.getErrorMsg() );
        Assert.assertTrue( listVersionWithPerfixAndDelimiter.isSuccess(),
                listVersionWithPerfixAndDelimiter.getErrorMsg() );

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                UserUtils.deleteUser( userName );
            }
        } catch ( BaseException e ) {
            Assert.fail( "clean up failed:" + e.getMessage() );
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }

    private class ListVersionThread extends S3ThreadBase {
        @Override
        public void exec() throws Exception {
            List<String> actVersionsKeyName = new ArrayList<String>();
            AmazonS3 s3Client = CommLib
                    .buildS3Client( acessKeys[ 0 ], acessKeys[ 1 ] );
            try {
                ListVersionsRequest req = new ListVersionsRequest()
                        .withBucketName( bucketName );
                VersionListing versionList = s3Client.listVersions( req );
                while ( true ) {
                    List<S3VersionSummary> verList = versionList
                            .getVersionSummaries();
                    for ( S3VersionSummary s3VersionSummary : verList ) {
                        actVersionsKeyName.add( s3VersionSummary.getKey() );
                    }

                    if ( versionList.isTruncated() ) {
                        versionList = s3Client
                                .listNextBatchOfVersions( versionList );
                    } else {
                        break;
                    }
                }
                Assert.assertEquals( actVersionsKeyName, expresultList1,
                        "the returned result by versions is wrong, act: "
                                + actVersionsKeyName.toString() + ", exp: "
                                + expresultList1.toString() );
            } finally {
                if ( s3Client != null ) {
                    s3Client.shutdown();
                }
            }
        }
    }

    private class ListVersionWithPerfixThread extends S3ThreadBase {
        @Override
        public void exec() throws Exception {
            List<String> actVersionsKeyName = new ArrayList<String>();
            AmazonS3 s3Client = CommLib
                    .buildS3Client( acessKeys[ 0 ], acessKeys[ 1 ] );
            try {
                ListVersionsRequest req = new ListVersionsRequest()
                        .withBucketName( bucketName ).withPrefix( prefix );
                VersionListing versionList = s3Client.listVersions( req );
                while ( true ) {
                    List<S3VersionSummary> verList = versionList
                            .getVersionSummaries();
                    for ( S3VersionSummary s3VersionSummary : verList ) {
                        actVersionsKeyName.add( s3VersionSummary.getKey() );
                    }

                    if ( versionList.isTruncated() ) {
                        versionList = s3Client
                                .listNextBatchOfVersions( versionList );
                    } else {
                        break;
                    }
                }
                Assert.assertEquals( actVersionsKeyName, expresultList2,
                        "the returned result by versions is wrong, act: "
                                + actVersionsKeyName.toString() + ", exp: "
                                + expresultList2.toString() );
            } finally {
                if ( s3Client != null ) {
                    s3Client.shutdown();
                }
            }
        }
    }

    private class ListVersionWithPerfixAndDelimiterThread extends S3ThreadBase {
        @Override
        public void exec() throws Exception {
            List<String> actCommonPrefixes = new ArrayList<String>();
            AmazonS3 s3Client = CommLib
                    .buildS3Client( acessKeys[ 0 ], acessKeys[ 1 ] );
            try {
                ListVersionsRequest req = new ListVersionsRequest()
                        .withBucketName( bucketName ).withPrefix( prefix )
                        .withDelimiter( delimiter );
                VersionListing versionList = s3Client.listVersions( req );
                while ( true ) {
                    List<String> commprefixesResult = versionList
                            .getCommonPrefixes();
                    for ( String s : commprefixesResult ) {
                        actCommonPrefixes.add( s );
                    }

                    if ( versionList.isTruncated() ) {
                        versionList = s3Client
                                .listNextBatchOfVersions( versionList );
                    } else {
                        break;
                    }
                }
                Assert.assertEquals( actCommonPrefixes, expresultList3,
                        "the returned result by commonperfixes is wrong, act: "
                                + actCommonPrefixes.toString() + ", exp: "
                                + expresultList3.toString() );
            } finally {
                if ( s3Client != null ) {
                    s3Client.shutdown();
                }
            }
        }
    }
}
