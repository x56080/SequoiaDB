package com.sequoias3.object;

import java.io.File;
import java.io.IOException;
import java.net.URI;
import java.net.URL;
import java.util.Calendar;
import java.util.Date;
import java.util.concurrent.atomic.AtomicInteger;

import org.springframework.core.io.Resource;
import org.springframework.http.RequestEntity;
import org.springframework.http.ResponseEntity;
import org.springframework.web.client.HttpClientErrorException;
import org.springframework.web.client.RestTemplate;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.BucketVersioningConfiguration;
import com.amazonaws.services.s3.model.GeneratePresignedUrlRequest;
import com.amazonaws.util.Md5Utils;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.ObjectUtils;

/**
 * @Description: seqDB-23216:通过预签名的url获取对象
 * @author fanyu
 * @Date:2021/1/7
 * @version:1.0
 */

public class GetObjectByGeneratePresignedUrl23216 extends S3TestBase {
    private int expSuccessNum = 6;
    private AtomicInteger total = new AtomicInteger( 0 );
    private String bucketName1 = "bucket23216";
    private String bucketName2 = null;
    private String objectName1 = "object23216A";
    private String objectName2 = "object23216B";
    private String content = "23216";
    private int versionNum = 3;
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * 200;
    private File localPath = null;
    private String filePath = null;
    private RestTemplate restTemplate = new RestTemplate();

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator
                + TestTools.getClassName() );
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        filePath = localPath + File.separator + "localFile_" + fileSize
                + ".txt";
        TestTools.LocalFile.createFile( filePath, fileSize );
        s3Client = CommLib.buildS3Client();
        bucketName2 = S3TestBase.enableVerBucketName;
        s3Client.createBucket( bucketName1 );
        CommLib.setBucketVersioning( s3Client, bucketName1,
                BucketVersioningConfiguration.SUSPENDED );
        // 禁用版本控制桶下对象
        s3Client.putObject( bucketName1, objectName1, new File( filePath ) );
        s3Client.putObject( bucketName1, objectName2, content );
        // 开启版本控制桶下对象
        s3Client.putObject( bucketName2, objectName1, "" );
        s3Client.putObject( bucketName2, objectName1, content );
        s3Client.putObject( bucketName2, objectName1, new File( filePath ) );
    }

    // 有效期内获取对象、对象内容为文件、禁用版本控制
    @Test
    private void test1() throws Exception {
        Calendar calendar = Calendar.getInstance();
        calendar.setTimeInMillis( new Date().getTime() + 1000 * 60 * 60 );
        // 有效时长为1小时
        Date expireTime = calendar.getTime();
        GeneratePresignedUrlRequest request = new GeneratePresignedUrlRequest(
                bucketName1, objectName1 ).withExpiration( expireTime )
                        .withMethod( com.amazonaws.HttpMethod.GET );
        // 生成预签名
        URL url = s3Client.generatePresignedUrl( request );
        // 通过预签名获取对象
        RequestEntity< ? > requestEntity = new RequestEntity<>(
                org.springframework.http.HttpMethod.GET,
                new URI( url.toString() ) );
        ResponseEntity< Resource > resp = restTemplate.exchange( requestEntity,
                Resource.class );
        // 检查结果
        Assert.assertEquals(
                Md5Utils.md5AsBase64( resp.getBody().getInputStream() ),
                Md5Utils.md5AsBase64( new File( filePath ) ),
                "url = " + url.toString() );
        total.getAndIncrement();
    }

    // 有效期内获取对象、对象内容为字符串、禁用版本控制
    @Test
    private void test2() throws Exception {
        Calendar calendar = Calendar.getInstance();
        calendar.setTimeInMillis( new Date().getTime() + 1000 * 60 * 24 );
        // 有效时长为24小时
        Date expireTime = calendar.getTime();
        GeneratePresignedUrlRequest request = new GeneratePresignedUrlRequest(
                bucketName1, objectName2 ).withExpiration( expireTime )
                        .withMethod( com.amazonaws.HttpMethod.GET );
        // 生成预签名
        URL url = s3Client.generatePresignedUrl( request );
        // 通过预签名获取对象
        RequestEntity< ? > requestEntity = new RequestEntity<>(
                org.springframework.http.HttpMethod.GET,
                new URI( url.toString() ) );
        ResponseEntity< String > resp = restTemplate.exchange( requestEntity,
                String.class );
        // 检查结果
        Assert.assertEquals( resp.getBody(), content );
        total.getAndIncrement();
    }

    // 开启版本控制、文件大小为0、200K等
    @Test
    private void test3() throws Exception {
        Calendar calendar = Calendar.getInstance();
        calendar.setTimeInMillis( new Date().getTime() + 1000 * 60 * 24 );
        // 有效时长为24小时
        Date expireTime = calendar.getTime();
        for ( int i = 0; i < versionNum; i++ ) {
            GeneratePresignedUrlRequest request = new GeneratePresignedUrlRequest(
                    bucketName2, objectName1 ).withVersionId( "" + i )
                            .withExpiration( expireTime )
                            .withMethod( com.amazonaws.HttpMethod.GET );
            // 生成预签名
            URL url = s3Client.generatePresignedUrl( request );
            // 通过预签名获取对象
            RequestEntity< ? > requestEntity = new RequestEntity<>(
                    org.springframework.http.HttpMethod.GET,
                    new URI( url.toString() ) );
            ResponseEntity< Resource > resp = restTemplate
                    .exchange( requestEntity, Resource.class );
            // 检查结果
            if ( i == 0 ) {
                Assert.assertEquals( resp.getBody(), null,
                        "url = " + url.toString() );
            } else if ( i == 1 ) {
                Assert.assertEquals(
                        Md5Utils.md5AsBase64( resp.getBody().getInputStream() ),
                        Md5Utils.md5AsBase64( content.getBytes() ),
                        "url = " + url.toString() );
            } else {
                Assert.assertEquals(
                        Md5Utils.md5AsBase64( resp.getBody().getInputStream() ),
                        Md5Utils.md5AsBase64( new File( filePath ) ),
                        "url = " + url.toString() );
            }
        }
        total.getAndIncrement();
    }

    // 过期后获取对象
    @Test
    private void test4() throws Exception {
        Calendar calendar = Calendar.getInstance();
        calendar.setTimeInMillis( new Date().getTime() + 1000 );
        // 有效时长为1秒
        Date expireTime = calendar.getTime();
        // 生成预签名
        URL url = s3Client.generatePresignedUrl( bucketName1, objectName1,
                expireTime );
        // 休眠5秒，使其过期
        int interval = 1000;
        int count = 60;
        boolean flag = false;
        // 通过预签名获取对象
        while ( count-- > 0 ) {
            RequestEntity< ? > requestEntity = new RequestEntity<>(
                    org.springframework.http.HttpMethod.GET,
                    new URI( url.toString() ) );
            try {
                restTemplate.exchange( requestEntity, Resource.class );
                Thread.sleep( interval );
            } catch ( HttpClientErrorException e ) {
                flag = true;
                if ( e.getStatusCode().value() != 403 ) {
                    throw new Exception( e.getResponseBodyAsString() );
                }
            }
            if ( flag ) {
                break;
            }
        }
        if ( !flag ) {
            throw new Exception( "time out!!! url = " + url.toString() );
        }
        total.getAndIncrement();
    }

    // bucketName或者keyName不存在
    @Test
    private void test5() throws Exception {
        Calendar calendar = Calendar.getInstance();
        calendar.setTimeInMillis( new Date().getTime() + 1000 );
        // 有效时长为1秒
        Date expireTime = calendar.getTime();
        // 桶不存在，生成预签名
        URL url1 = s3Client.generatePresignedUrl( bucketName1 + "A",
                objectName1, expireTime );
        RequestEntity< ? > requestEntity1 = new RequestEntity<>(
                org.springframework.http.HttpMethod.GET,
                new URI( url1.toString() ) );
        try {
            restTemplate.exchange( requestEntity1, Resource.class );
            Assert.fail( "exp failed but act success!!!" );
        } catch ( HttpClientErrorException e ) {
            if ( e.getStatusCode().value() != 404
                    && e.getStatusCode().value() != 403 ) {
                throw new Exception( e.getResponseBodyAsString() );
            }
        }

        // 对象不存在，生成预签名
        URL url2 = s3Client.generatePresignedUrl( bucketName1,
                objectName1 + "A", expireTime );
        RequestEntity< ? > requestEntity2 = new RequestEntity<>(
                org.springframework.http.HttpMethod.GET,
                new URI( url2.toString() ) );
        try {
            restTemplate.exchange( requestEntity2, Resource.class );
            Assert.fail( "exp failed but act success!!!" );
        } catch ( HttpClientErrorException e ) {
            if ( e.getStatusCode().value() != 404
                    && e.getStatusCode().value() != 403 ) {
                throw new Exception( e.getResponseBodyAsString() );
            }
        }
        total.getAndIncrement();
    }

    // expireTime小于当前时间
    @Test
    private void test6() throws Exception {
        Calendar calendar = Calendar.getInstance();
        calendar.setTimeInMillis( new Date().getTime() - 24 * 60 * 60 * 1000 );
        // 有效时长为1秒
        Date expireTime = calendar.getTime();
        URL url1 = s3Client.generatePresignedUrl( bucketName1, objectName1,
                expireTime );
        RequestEntity< ? > requestEntity1 = new RequestEntity<>(
                org.springframework.http.HttpMethod.GET,
                new URI( url1.toString() ) );
        try {
            restTemplate.exchange( requestEntity1, Resource.class );
            Assert.fail( "exp failed but act success!!!" );
        } catch ( HttpClientErrorException e ) {
            if ( e.getStatusCode().value() != 403 ) {
                throw new Exception( e.getResponseBodyAsString() );
            }
        }
        total.getAndIncrement();
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( total.get() == expSuccessNum ) {
                CommLib.clearBucket( s3Client, bucketName1 );
                ObjectUtils.deleteObjectAllVersions( s3Client, bucketName2,
                        objectName1 );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }
}
