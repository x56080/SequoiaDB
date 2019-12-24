package com.sequoias3.head;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.ObjectMetadata;
import com.amazonaws.services.s3.model.PutObjectResult;
import com.sequoiadb.exception.BaseException;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.UserUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.ArrayList;
import java.util.List;
import java.util.Random;

/**
 * test content: head查询对象接口参数校验 testlink-case: seqDB-16708
 *
 * @author wangkexin
 * @Date 2018.12.07
 * @version 1.00
 */

public class TestDoesObjectExist16708 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket16708";
    private String userName = "user16708";
    private String roleName = "normal";
    private String keyName = "key16708";
    private String content = "content16708";
    private List<String> validValues = new ArrayList<>();
    private AmazonS3 s3Client = null;

    @BeforeClass
    private void setUp() throws Exception {
        CommLib.clearUser( userName );
        String[] accessKeys = UserUtils.createUser( userName, roleName );
        s3Client = CommLib.buildS3Client( accessKeys[ 0 ], accessKeys[ 1 ] );
    }

    @Test
    private void testDoesObjectExist() throws Exception {
        validValues.add( getRandomString( 3 ) );
        validValues.add( getRandomString( 63 ) );

        // test a :指定桶名为合法名 长度边界值为：3个字符，63个字符
        for ( String validname : validValues ) {
            s3Client.createBucket( validname );
            PutObjectResult objectResult = s3Client
                    .putObject( validname, keyName, content + validname );
            Assert.assertTrue( s3Client.doesObjectExist( validname, keyName ) );
            ObjectMetadata metadata = s3Client
                    .getObjectMetadata( validname, keyName );
            Assert.assertEquals( metadata.getETag(), objectResult.getETag() );
        }

        // test b :指定对象名为合法名 长度边界值为：900个字符
        keyName = getRandomString( 900 );
        s3Client.createBucket( bucketName );
        PutObjectResult objectResult = s3Client
                .putObject( bucketName, keyName, content + "testb" );
        Assert.assertTrue( s3Client.doesObjectExist( bucketName, keyName ) );
        ObjectMetadata metadata = s3Client
                .getObjectMetadata( bucketName, keyName );
        Assert.assertEquals( metadata.getETag(), objectResult.getETag() );

        // test c :指定桶名，对象名分别为null和空串""
        try {
            s3Client.doesObjectExist( null, keyName );
            Assert.fail(
                    "test c doesObjectExist:bucket name is illegal should fail" );
        } catch ( IllegalArgumentException e ) {
            Assert.assertTrue( e.getMessage().equals(
                    "The bucket name parameter must be specified when requesting an object's metadata" ) );
        }

        try {
            Assert.assertFalse( s3Client.doesObjectExist( bucketName, null ) );
            Assert.fail(
                    "test c doesObjectExist:key name is illegal should fail" );
        } catch ( IllegalArgumentException e ) {
            Assert.assertTrue( e.getMessage().equals(
                    "The key parameter must be specified when requesting an object's metadata" ) );
        }

        Assert.assertFalse( s3Client.doesObjectExist( "", keyName ) );
        Assert.assertTrue( s3Client.doesObjectExist( bucketName, "" ) );

        try {
            s3Client.getObjectMetadata( null, keyName );
            Assert.fail(
                    "test c getObjectMetadata:bucket name is illegal should fail" );
        } catch ( IllegalArgumentException e ) {
            Assert.assertTrue( e.getMessage().equals(
                    "The bucket name parameter must be specified when requesting an object's metadata" ) );
        }

        try {
            s3Client.getObjectMetadata( bucketName, null );
            Assert.fail(
                    "test c getObjectMetadata:bucket name is illegal should fail" );
        } catch ( IllegalArgumentException e ) {
            Assert.assertTrue( e.getMessage().equals(
                    "The key parameter must be specified when requesting an object's metadata" ) );
        }

        try {
            s3Client.getObjectMetadata( "", keyName );
            Assert.fail(
                    "test c getObjectMetadata:bucket name is \"\" should fail" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getStatusCode(), 404 );

        }

        metadata = s3Client.getObjectMetadata( bucketName, "" );
        Assert.assertEquals( metadata.getETag(), null );
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
