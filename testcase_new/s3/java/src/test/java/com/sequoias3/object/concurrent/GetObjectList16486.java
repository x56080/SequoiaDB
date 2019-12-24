package com.sequoias3.object.concurrent;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.ListObjectsRequest;
import com.amazonaws.services.s3.model.ListObjectsV2Request;
import com.amazonaws.services.s3.model.ListObjectsV2Result;
import com.amazonaws.services.s3.model.ObjectListing;
import com.amazonaws.services.s3.model.S3ObjectSummary;
import com.sequoiadb.exception.BaseException;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.S3ThreadBase;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import com.sequoias3.testcommon.s3utils.UserUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/**
 * test content: 并发不同条件查询对象列表,覆盖listObjectV1和listObjectV2 testlink-case:
 * seqDB-16486
 *
 * @author wangkexin
 * @Date 2019.01.03
 * @version 1.00
 */
public class GetObjectList16486 extends S3TestBase {
    private String bucketName = "bucket16486";
    private String userName = "user16486";
    private String roleName = "normal";
    private String keyName = "/dir/dir";
    private String prefix = "/dir";
    private String delimiter = "/";
    private List<String> expresultList1 = new ArrayList<String>();
    private List<String> expresultList2 = new ArrayList<String>();
    private List<String> expresultList3 = new ArrayList<String>();
    private int objectTotalNum = 100;
    private AmazonS3 s3Client = null;
    private String[] acessKeys = null;
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws Exception {
        CommLib.clearUser( userName );
        acessKeys = UserUtils.createUser( userName, roleName );
        s3Client = CommLib.buildS3Client( acessKeys[ 0 ], acessKeys[ 1 ] );
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );

        // put multiple objects
        for ( int i = 0; i < objectTotalNum; i++ ) {
            String currentKeyName = keyName + i + "/16486";
            s3Client.putObject( bucketName, currentKeyName,
                    "object_file16486" );
            expresultList1.add( currentKeyName );
            expresultList2.add( currentKeyName );
        }

        // put another objects that do not match prefix
        s3Client.putObject( bucketName, "/testa16486", "object_file16486" );
        s3Client.putObject( bucketName, "/testb16486", "object_file16486" );
        expresultList1.add( "/testa16486" );
        expresultList1.add( "/testb16486" );

        Collections.sort( expresultList1 );
        Collections.sort( expresultList2 );
        expresultList3.add( "/dir/" );
    }

    @Test
    public void testGetObjectList() throws Exception {
        ListObjectThread listObject = new ListObjectThread();
        ListObjectWithPerfixThread listObjectWithPerfix = new ListObjectWithPerfixThread();
        ListObjectWithPerfixAndDelimiterThread listObjectWithPerfixAndDelimiter = new ListObjectWithPerfixAndDelimiterThread();
        ListObjectV1Thread listObjectV1 = new ListObjectV1Thread();
        ListObjectV1WithPerfixAndDelimiterThread listObjectV1WithPerfixAndDelimiter = new ListObjectV1WithPerfixAndDelimiterThread();

        listObject.start();
        listObjectWithPerfix.start();
        listObjectWithPerfixAndDelimiter.start();
        listObjectV1.start();
        listObjectV1WithPerfixAndDelimiter.start();

        Assert.assertTrue( listObject.isSuccess(), listObject.getErrorMsg() );
        Assert.assertTrue( listObjectV1.isSuccess(),
                listObjectV1.getErrorMsg() );
        Assert.assertTrue( listObjectWithPerfix.isSuccess(),
                listObjectWithPerfix.getErrorMsg() );
        Assert.assertTrue( listObjectWithPerfixAndDelimiter.isSuccess(),
                listObjectWithPerfixAndDelimiter.getErrorMsg() );
        Assert.assertTrue( listObjectV1WithPerfixAndDelimiter.isSuccess(),
                listObjectV1WithPerfixAndDelimiter.getErrorMsg() );

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                deleteObjectsAndBucket();
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

    private void deleteObjectsAndBucket() {
        for ( int i = 0; i < expresultList1.size(); i++ ) {
            s3Client.deleteObject( bucketName, expresultList1.get( i ) );
        }
        s3Client.deleteBucket( bucketName );
    }

    private class ListObjectThread extends S3ThreadBase {
        @Override
        public void exec() throws Exception {
            AmazonS3 s3Client = CommLib
                    .buildS3Client( acessKeys[ 0 ], acessKeys[ 1 ] );
            try {
                List<S3ObjectSummary> contentsResult = new ArrayList<>();
                ListObjectsV2Request req = new ListObjectsV2Request()
                        .withBucketName( bucketName );
                ListObjectsV2Result result;

                do {
                    result = s3Client.listObjectsV2( req );
                    contentsResult.addAll( result.getObjectSummaries() );
                    String nextContinuationToken = result
                            .getNextContinuationToken();
                    req.setContinuationToken( nextContinuationToken );
                } while ( result.isTruncated() );

                ObjectUtils.checkListObjectsV2KeyName( contentsResult,
                        expresultList1 );
            } finally {
                if ( s3Client != null ) {
                    s3Client.shutdown();
                }
            }
        }
    }

    private class ListObjectV1Thread extends S3ThreadBase {
        @Override
        public void exec() throws Exception {
            AmazonS3 s3Client = CommLib
                    .buildS3Client( acessKeys[ 0 ], acessKeys[ 1 ] );
            try {
                List<S3ObjectSummary> contentsResult = new ArrayList<>();
                ListObjectsRequest req = new ListObjectsRequest()
                        .withBucketName( bucketName );
                ObjectListing result;

                do {
                    result = s3Client.listObjects( req );
                    contentsResult.addAll( result.getObjectSummaries() );
                    String marker = result.getNextMarker();
                    req.setMarker( marker );
                } while ( result.isTruncated() );

                ObjectUtils.checkListObjectsV2KeyName( contentsResult,
                        expresultList1 );
            } finally {
                if ( s3Client != null ) {
                    s3Client.shutdown();
                }
            }
        }
    }

    private class ListObjectWithPerfixThread extends S3ThreadBase {
        @Override
        public void exec() throws Exception {
            AmazonS3 s3Client = CommLib
                    .buildS3Client( acessKeys[ 0 ], acessKeys[ 1 ] );
            try {
                List<S3ObjectSummary> contentsResult = new ArrayList<>();
                ListObjectsV2Request req = new ListObjectsV2Request()
                        .withBucketName( bucketName ).withPrefix( prefix );
                ListObjectsV2Result result;

                do {
                    result = s3Client.listObjectsV2( req );
                    contentsResult.addAll( result.getObjectSummaries() );
                    String nextContinuationToken = result
                            .getNextContinuationToken();
                    req.setContinuationToken( nextContinuationToken );
                } while ( result.isTruncated() );

                ObjectUtils.checkListObjectsV2KeyName( contentsResult,
                        expresultList2 );
            } finally {
                if ( s3Client != null ) {
                    s3Client.shutdown();
                }
            }
        }
    }

    private class ListObjectWithPerfixAndDelimiterThread extends S3ThreadBase {
        @Override
        public void exec() throws Exception {
            AmazonS3 s3Client = CommLib
                    .buildS3Client( acessKeys[ 0 ], acessKeys[ 1 ] );
            try {
                List<String> commprefixesResult = new ArrayList<>();
                ListObjectsV2Request req = new ListObjectsV2Request()
                        .withBucketName( bucketName ).withPrefix( prefix )
                        .withDelimiter( delimiter );
                ListObjectsV2Result result;

                do {
                    result = s3Client.listObjectsV2( req );
                    commprefixesResult.addAll( result.getCommonPrefixes() );
                    String nextContinuationToken = result
                            .getNextContinuationToken();
                    req.setContinuationToken( nextContinuationToken );
                } while ( result.isTruncated() );

                ObjectUtils.checkListObjectsV2Commprefixes( commprefixesResult,
                        expresultList3 );
            } finally {
                if ( s3Client != null ) {
                    s3Client.shutdown();
                }
            }
        }
    }

    private class ListObjectV1WithPerfixAndDelimiterThread
            extends S3ThreadBase {
        @Override
        public void exec() throws Exception {
            AmazonS3 s3Client = CommLib
                    .buildS3Client( acessKeys[ 0 ], acessKeys[ 1 ] );
            try {
                List<String> commprefixesResult = new ArrayList<>();
                ListObjectsRequest req = new ListObjectsRequest()
                        .withBucketName( bucketName ).withPrefix( prefix )
                        .withDelimiter( delimiter );
                ObjectListing result;

                do {
                    result = s3Client.listObjects( req );
                    commprefixesResult.addAll( result.getCommonPrefixes() );
                    String nextMarker = result.getNextMarker();
                    req.setMarker( nextMarker );
                } while ( result.isTruncated() );

                ObjectUtils.checkListObjectsV2Commprefixes( commprefixesResult,
                        expresultList3 );
            } finally {
                if ( s3Client != null ) {
                    s3Client.shutdown();
                }
            }
        }
    }
}
