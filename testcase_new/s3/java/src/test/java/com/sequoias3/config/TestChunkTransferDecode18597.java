package com.sequoias3.config;

import java.io.File;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.amazonaws.ClientConfiguration;
import com.amazonaws.auth.AWSCredentials;
import com.amazonaws.auth.AWSStaticCredentialsProvider;
import com.amazonaws.auth.BasicAWSCredentials;
import com.amazonaws.client.builder.AwsClientBuilder;
import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.AmazonS3ClientBuilder;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import com.sequoias3.testcommon.s3utils.UserUtils;
import com.sequoias3.testcommon.s3utils.bean.UserCommDefind;

/**
 * test content: 支持chunk解码，创建对象 testlink-case: seqDB-18597
 *
 * @author wangkexin
 * @Date 2019.06.26
 * @version 1.00
 */
public class TestChunkTransferDecode18597 extends S3TestBase {
    private boolean runSuccess = false;
    private String clientRegion = "us-east-1";
    private String bucketName = "bucket18597";
    private String userName = "user18597";
    private String keyName = "key18597";
    private int fileSize = 1024 * 128;
    private int updateSize = 1024 * 127;
    private int updateSize2 = 1024 * 129;
    private File localPath = null;
    private String filePath = null;
    private String updatePath = null;
    private String updatePath2 = null;
    private String[] accessKeys = null;
    private AmazonS3 s3Client = null;

    @BeforeClass
    private void setUp() throws Exception {
        localPath = new File( S3TestBase.workDir + File.separator
                + TestTools.getClassName() );
        filePath = localPath + File.separator + "localFile_" + fileSize
                + ".txt";
        updatePath = localPath + File.separator + "localFile_" + updateSize
                + ".txt";
        updatePath2 = localPath + File.separator + "localFile_" + updateSize2
                + ".txt";

        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath, fileSize );
        TestTools.LocalFile.createFile( updatePath, updateSize );
        TestTools.LocalFile.createFile( updatePath2, updateSize2 );

        CommLib.clearUser( userName );
        accessKeys = UserUtils.createUser( userName, UserCommDefind.normal );
        s3Client = buildS3Client( accessKeys[ 0 ], accessKeys[ 1 ] );
    }

    @Test
    private void testReputBacket() throws Exception {
        s3Client.createBucket( bucketName );
        s3Client.putObject( bucketName, keyName, new File( filePath ) );
        String downfileMd5 = ObjectUtils.getMd5OfObject( s3Client, localPath,
                bucketName, keyName );
        Assert.assertEquals( downfileMd5, TestTools.getMD5( filePath ) );

        // update object
        s3Client.putObject( bucketName, keyName, new File( updatePath ) );
        downfileMd5 = ObjectUtils.getMd5OfObject( s3Client, localPath,
                bucketName, keyName );
        Assert.assertEquals( downfileMd5, TestTools.getMD5( updatePath ) );

        // update object
        s3Client.putObject( bucketName, keyName, new File( updatePath2 ) );
        downfileMd5 = ObjectUtils.getMd5OfObject( s3Client, localPath,
                bucketName, keyName );
        Assert.assertEquals( downfileMd5, TestTools.getMD5( updatePath2 ) );

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( runSuccess ) {
                CommLib.clearUser( userName );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }

    public AmazonS3 buildS3Client( String ACCESS_KEY, String SECRET_KEY ) {
        AmazonS3 s3Client = null;
        AWSCredentials credentials = new BasicAWSCredentials( ACCESS_KEY,
                SECRET_KEY );
        AwsClientBuilder.EndpointConfiguration endpointConfiguration = new AwsClientBuilder.EndpointConfiguration(
                S3TestBase.s3ClientUrl, clientRegion );
        ClientConfiguration config = new ClientConfiguration();
        config.setUseExpectContinue( false );
        config.setSocketTimeout( 300000 );
        s3Client = AmazonS3ClientBuilder.standard()
                .withEndpointConfiguration( endpointConfiguration )
                .withClientConfiguration( config )
                .withChunkedEncodingDisabled( false )
                .withPathStyleAccessEnabled( true )
                .withCredentials(
                        new AWSStaticCredentialsProvider( credentials ) )
                .build();
        return s3Client;
    }
}
