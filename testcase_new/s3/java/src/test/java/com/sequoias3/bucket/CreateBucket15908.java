package com.sequoias3.bucket;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.Bucket;
import com.amazonaws.services.s3.model.Owner;
import com.sequoiadb.exception.BaseException;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.RestClient;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.UserUtils;
import org.apache.http.client.methods.HttpPut;
import org.apache.http.impl.client.CloseableHttpClient;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.UnsupportedEncodingException;
import java.net.URLDecoder;
import java.net.URLEncoder;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Random;

/**
 * @Description: bucket name parameter verification testlink-case: seqDB-15908
 *
 * @author wangkexin
 * @Date 2018.09.28
 * @version 1.00
 */
public class CreateBucket15908 extends S3TestBase {
    private static CloseableHttpClient client;
    private boolean runSuccess = false;
    private List< String > bucketNameList = new ArrayList< String >();
    private List< String > illegalbucketNameList = new ArrayList< String >();
    private String userName = "user15908";
    private String roleName = "normal";
    private AmazonS3 s3Client = null;
    private String[] accessKeys = null;

    @BeforeClass
    private void setUp() throws Exception {
        CommLib.clearUser( userName );
        accessKeys = UserUtils.createUser( userName, roleName );
        s3Client = CommLib.buildS3Client( accessKeys[ 0 ], accessKeys[ 1 ] );
    }

    @SuppressWarnings("deprecation")
    @Test
    private void testCreateBucket() throws Exception {
        bucketNameList.add( "testbucket" );
        bucketNameList
                .add( URLEncoder.encode( getRandomString( 3 ), "UTF-8" ) );
        bucketNameList
                .add( URLEncoder.encode( getRandomString( 63 ), "UTF-8" ) );
        bucketNameList
                .add( URLEncoder.encode( "~`!@#$%^\"&*()-=+;?<>", "UTF-8" ) );
        bucketNameList.add( URLEncoder.encode( "Test_Bucket#001$A", "UTF-8" ) );
        bucketNameList.add( ".testbucket01" );
        bucketNameList.add( "test.bucket02" );
        bucketNameList.add( "testbucket03." );
        bucketNameList.add( "test....bucket04" );
        bucketNameList.add( "test-bucket05" );
        bucketNameList.add( "-testbucket06" );
        bucketNameList.add( "testbucket07-" );
        bucketNameList.add( "users" );
        bucketNameList.add( "region" );
        for ( int i = 0; i < bucketNameList.size(); i++ ) {
            createBucket( bucketNameList.get( i ) );
        }
        checkbucketNameListResult( bucketNameList.size() );

        illegalbucketNameList
                .add( URLEncoder.encode( getRandomString( 2 ), "UTF-8" ) );
        illegalbucketNameList
                .add( URLEncoder.encode( getRandomString( 64 ), "UTF-8" ) );

        for ( int i = 0; i < illegalbucketNameList.size(); i++ ) {
            try {
                createBucket( illegalbucketNameList.get( i ) );
                Assert.fail( "create bucket should fail!" );
            } catch ( Exception e ) {
                String str = e.getMessage().replace( "</Code>", "<Code>" );
                String[] strs = str.split( "<Code>" );
                Assert.assertEquals( strs[ 1 ], "InvalidBucketName" );
            }
            Assert.assertEquals( false, s3Client.doesBucketExist(
                    illegalbucketNameList.get( i ).toLowerCase() ) );
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

    private void createBucket( String bucketNameList ) throws Exception {
        HttpPut request = new HttpPut(
                S3TestBase.s3ClientUrl + "/" + bucketNameList );
        request.setHeader( "Authorization",
                "Credential=" + accessKeys[ 0 ] + "/" );
        client = RestClient.createHttpClient();
        RestClient.sendRequest( client, request );
    }

    private void checkbucketNameListResult( int bucketNums )
            throws UnsupportedEncodingException {
        // create one bucket,check the bucket name and owner name
        List< Bucket > buckets = s3Client.listBuckets();
        // check bucket number
        Assert.assertEquals( buckets.size(), bucketNums );

        List< String > actbucketNameLists = new ArrayList<>();
        for ( Bucket bucket : buckets ) {
            Owner actOwner = bucket.getOwner();
            Assert.assertEquals( actOwner.getDisplayName(), userName );
            actbucketNameLists.add( bucket.getName() );
        }
        Collections.sort( actbucketNameLists );

        List< String > expBucketNames = new ArrayList<>();
        for ( String bucket : bucketNameList ) {
            expBucketNames
                    .add( URLDecoder.decode( bucket, "UTF-8" ).toLowerCase() );
        }

        Collections.sort( expBucketNames );
        Assert.assertEquals( actbucketNameLists, expBucketNames );
    }

    private String getRandomString( int length ) {
        String str = "zxcvbnmlkjhgfdsaqwertyuiopQWERTYUIOPLKJHGFDSAZXCVBNM1234567890";
        Random random = new Random();
        StringBuffer sbuff = new StringBuffer();
        for ( int i = 0; i < length; i++ ) {
            int number = random.nextInt( 62 );
            sbuff.append( str.charAt( number ) );
        }
        return sbuff.toString();
    }
}
