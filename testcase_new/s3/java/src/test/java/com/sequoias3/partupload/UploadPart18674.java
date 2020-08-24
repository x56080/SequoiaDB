package com.sequoias3.partupload;

import com.amazonaws.ClientConfiguration;
import com.amazonaws.auth.AWSCredentials;
import com.amazonaws.auth.AWSStaticCredentialsProvider;
import com.amazonaws.auth.BasicAWSCredentials;
import com.amazonaws.client.builder.AwsClientBuilder;
import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.AmazonS3ClientBuilder;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.PartETag;
import com.amazonaws.services.s3.model.UploadPartRequest;
import com.amazonaws.services.s3.model.UploadPartResult;
import com.amazonaws.util.Base64;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import com.sequoias3.testcommon.s3utils.PartUploadUtils;
import org.apache.commons.codec.binary.Hex;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.security.MessageDigest;
import java.util.ArrayList;
import java.util.List;

/**
 * @Description: 上传一个分段 testlink-case: seqDB-18674
 *
 * @author wangkexin
 * @Date 2019.7.25
 * @version 1.00
 */
public class UploadPart18674 extends S3TestBase {
    private String AWS_ACCESS_KEY = "ABCDEFGHIJKLMNOPQRST";
    private String AWS_SECRET_KEY = "abcdefghijklmnopqrstuvwxyz0123456789ABCD";
    private String clientRegion = "us-east-1";
    private boolean runSuccess = false;
    private String bucketName = "bucket18674";
    private AmazonS3 s3Client = null;
    private long fileSize = 6 * 1024 * 1024;
    private File localPath = null;
    private File file = null;
    private String filePath = null;

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator
                + TestTools.getClassName() );
        filePath = localPath + File.separator + "localFile_" + fileSize
                + ".txt";

        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath, fileSize );
        file = new File( filePath );

        s3Client = buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );
    }

    @Test
    private void testUpload() throws Exception {
        String keyName = "/aa/bb/object18674";

        // 指定content-Md5分段长度等于对象长度
        long partSize = 6 * 1024 * 1024;
        testUpload1( keyName + "a", partSize );

        // 分段长度大于对象长度
        partSize = 7 * 1024 * 1024;
        testUpload2( keyName + "b", partSize );

        // 分段长度小于对象长度
        partSize = 5 * 1024 * 1024;
        testUpload3( keyName + "c", partSize );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                CommLib.clearBucket( s3Client, bucketName );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            s3Client.shutdown();
        }
    }

    private void testUpload1( String keyName, long setPartSize )
            throws Exception {
        String uploadId = PartUploadUtils.initPartUpload( s3Client, bucketName,
                keyName );

        String md5[] = null;
        List< PartETag > partEtags = new ArrayList<>();
        md5 = getMD5s( file, 0, setPartSize );
        String contentMd5 = md5[ 0 ];

        // 指定content-Md5上传一个分段
        UploadPartRequest partRequest = new UploadPartRequest().withFile( file )
                .withFileOffset( 0 ).withPartNumber( 1 )
                .withPartSize( setPartSize ).withBucketName( bucketName )
                .withKey( keyName ).withUploadId( uploadId )
                .withMD5Digest( contentMd5 );
        UploadPartResult uploadPartResult = s3Client.uploadPart( partRequest );
        partEtags.add( uploadPartResult.getPartETag() );
        String expPartMd5 = md5[ 1 ];
        String actPartMd5 = uploadPartResult.getPartETag().getETag();
        Assert.assertEquals( actPartMd5, expPartMd5, "part number = 1" );

        PartUploadUtils.completeMultipartUpload( s3Client, bucketName, keyName,
                uploadId, partEtags );
        String expMd5 = TestTools.getMD5( filePath );
        String actMd5 = ObjectUtils.getMd5OfObject( s3Client, localPath,
                bucketName, keyName );
        Assert.assertEquals( actMd5, expMd5 );
    }

    private void testUpload2( String keyName, long setPartSize )
            throws IOException {
        String uploadId = PartUploadUtils.initPartUpload( s3Client, bucketName,
                keyName );

        UploadPartRequest partRequest = new UploadPartRequest().withFile( file )
                .withFileOffset( 0 ).withPartNumber( 1 )
                .withPartSize( setPartSize ).withBucketName( bucketName )
                .withKey( keyName ).withUploadId( uploadId );
        try {
            s3Client.uploadPart( partRequest );
            Assert.fail( "upload should fail." );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "IncompleteBody" );
        }

        Assert.assertFalse( s3Client.doesObjectExist( bucketName, keyName ) );
    }

    private void testUpload3( String keyName, long setPartSize )
            throws Exception {
        String uploadId = PartUploadUtils.initPartUpload( s3Client, bucketName,
                keyName );

        List< PartETag > partEtags = new ArrayList<>();
        String[] md5 = getMD5s( file, 0, setPartSize );
        UploadPartRequest partRequest = new UploadPartRequest().withFile( file )
                .withFileOffset( 0 ).withPartNumber( 1 )
                .withPartSize( setPartSize ).withBucketName( bucketName )
                .withKey( keyName ).withUploadId( uploadId );
        UploadPartResult uploadPartResult = s3Client.uploadPart( partRequest );
        partEtags.add( uploadPartResult.getPartETag() );
        String expPartMd5 = md5[ 1 ];
        String actPartMd5 = uploadPartResult.getPartETag().getETag();
        Assert.assertEquals( actPartMd5, expPartMd5, "part number = "
                + uploadPartResult.getPartETag().getPartNumber() );

        PartUploadUtils.completeMultipartUpload( s3Client, bucketName, keyName,
                uploadId, partEtags );
        String expMd5 = getMD5s( file, 0, setPartSize )[ 1 ];
        String downloadMd5 = ObjectUtils.getMd5OfObject( s3Client, localPath,
                bucketName, keyName );
        Assert.assertEquals( downloadMd5, expMd5 );
    }

    private String[] getMD5s( File file, long offset, long partsize )
            throws IOException {
        String[] md5s = new String[ 2 ];
        FileInputStream fileInputStream = null;
        int length = ( int ) file.length();
        try {
            MessageDigest md5 = MessageDigest.getInstance( "MD5" );
            fileInputStream = new FileInputStream( file );
            byte[] buffer = new byte[ length ];
            if ( fileInputStream.read( buffer ) != -1 ) {
                md5.update( buffer, ( int ) offset, ( int ) partsize );
            }

            byte[] digest = md5.digest();
            // 请求中携带md5需经过base64加密
            md5s[ 0 ] = Base64.encodeAsString( digest );
            // 文件指定部分的md5值
            md5s[ 1 ] = new String( Hex.encodeHex( digest ) );
            return md5s;
        } catch ( Exception e ) {
            e.printStackTrace();
            return null;
        } finally {
            if ( fileInputStream != null ) {
                fileInputStream.close();
            }
        }
    }

    private AmazonS3 buildS3Client() {
        // 分段长度大于对象长度时需开启chunk
        AmazonS3 s3Client = null;
        AWSCredentials credentials = new BasicAWSCredentials( AWS_ACCESS_KEY,
                AWS_SECRET_KEY );
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
