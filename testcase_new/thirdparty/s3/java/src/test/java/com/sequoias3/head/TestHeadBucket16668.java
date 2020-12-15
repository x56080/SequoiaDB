package com.sequoias3.head;

import com.amazonaws.AmazonServiceException;
import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.HeadBucketRequest;
import com.sequoiadb.exception.BaseException;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.UserUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.Random;

/**
 * @Description: head查询桶接口参数校验 testlink-case: seqDB-16668
 *
 * @author wangkexin
 * @Date 2018.12.07
 * @version 1.00
 */

public class TestHeadBucket16668 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket16668";
    private String userName = "user16668";
    private String roleName = "normal";
    private AmazonS3 s3Client = null;

    @BeforeClass
    private void setUp() throws Exception {
        CommLib.clearUser( userName );
        String[] accessKeys = UserUtils.createUser( userName, roleName );
        s3Client = CommLib.buildS3Client( accessKeys[ 0 ], accessKeys[ 1 ] );
    }

    @SuppressWarnings("deprecation")
    @Test
    private void testDoesObjectExist() throws Exception {
        // test a :指定桶名为合法名 长度边界值为：3个字符，63个字符
        bucketName = getRandomString( 3 );
        s3Client.createBucket( bucketName );
        s3Client.headBucket( new HeadBucketRequest( bucketName ) );
        Assert.assertTrue( s3Client.doesBucketExist( bucketName ) );

        bucketName = getRandomString( 63 );
        s3Client.createBucket( bucketName );
        s3Client.headBucket( new HeadBucketRequest( bucketName ) );
        Assert.assertTrue( s3Client.doesBucketExist( bucketName ) );

        // test b :指定桶名为null和空串""
        try {
            s3Client.headBucket( new HeadBucketRequest( null ) );
            Assert.fail(
                    "test b headBucket:bucket name is illegal should fail" );
        } catch ( IllegalArgumentException e ) {
            Assert.assertTrue( e.getMessage()
                    .equals( "The bucketName parameter must be specified." ) );
        }

        try {
            s3Client.headBucket( new HeadBucketRequest( "" ) );
            Assert.fail( "test b headBucket:bucket name is '' should fail" );
        } catch ( AmazonServiceException e ) {
            Assert.assertEquals( e.getStatusCode(), 405 );
        }

        try {
            Assert.assertFalse( s3Client.doesBucketExist( null ) );
            Assert.fail(
                    "test b doesBucketExist:bucket name is illegal should fail" );
        } catch ( IllegalArgumentException e ) {
            Assert.assertTrue( e.getMessage()
                    .equals( "The bucketName parameter must be specified." ) );
        }

        try {
            Assert.assertTrue( s3Client.doesBucketExist( "" ) );
            Assert.fail(
                    "test b doesBucketExist:bucket name is '' should fail" );
        } catch ( AmazonServiceException e ) {
            Assert.assertEquals( e.getStatusCode(), 405 );
        }

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
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

    private String getRandomString( int length ) {
        String str = "zxcvbnmlkjhgfdsaqwertyuiop1234567890";
        Random random = new Random();
        StringBuffer sbuff = new StringBuffer();
        for ( int i = 0; i < length; i++ ) {
            int number = random.nextInt( 36 );
            sbuff.append( str.charAt( number ) );
        }
        return sbuff.toString();
    }
}
