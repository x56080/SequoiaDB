package com.sequoias3.head;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.PutObjectResult;
import com.sequoiadb.exception.BaseException;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.RestClient;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.UserUtils;
import org.apache.http.client.methods.CloseableHttpResponse;
import org.apache.http.client.methods.HttpHead;
import org.apache.http.impl.client.CloseableHttpClient;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import java.io.File;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * test content: 不带versionId查询对象，指定匹配range范围 testlink-case: seqDB-16680
 *
 * @author wangkexin
 * @Date 2018.12.07
 * @version 1.00
 */

public class TestGetObjectMetadata16680 extends S3TestBase {
    private static CloseableHttpClient client;
    private AtomicInteger actSuccessTests = new AtomicInteger( 0 );
    private String bucketName = "bucket16680";
    private String userName = "user16680";
    private String roleName = "normal";
    private String keyName = "key16680";
    private int fileSize = 1024 * 100;
    private String expEtag = null;
    private File localPath = null;
    private String filePath = null;
    private String[] accessKeys = null;
    private AmazonS3 s3Client = null;

    @DataProvider(name = "rangeProvider", parallel = true)
    public Object[][] generatePageSize() {
        return new Object[][] {
                // the parameter : begin and end, fileSize is 1024 * 100
                // test a : 指定range为起始位置0
                new Object[] { "0", "0", 1 },
                // test b : 指定range为结束位置99k、100k
                new Object[] { "102398", "102398", 1 },
                new Object[] { "102399", "102399", 1 },
                // test c : 指定range为中间范围100k-100k+1byte(超过边界值)、10k-99k
                new Object[] { "102399", "102400", 1 },
                new Object[] { "10240", "101376",
                        ( ( 99 * 1024 ) - ( 10 * 1024 ) ) + 1 },
                // test d : 指定range起始范围超过对象最大长度(100k),设置为199k
                new Object[] { "203776", "203776", 0 } };
    }

    @BeforeClass
    private void setUp() throws Exception {
        CommLib.clearUser( userName );
        localPath = new File( S3TestBase.workDir + File.separator
                + TestTools.getClassName() );
        filePath = localPath + File.separator + "localFile_" + fileSize
                + ".txt";
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath, fileSize );
        accessKeys = UserUtils.createUser( userName, roleName );
        s3Client = CommLib.buildS3Client( accessKeys[ 0 ], accessKeys[ 1 ] );

        s3Client.createBucket( bucketName );
        PutObjectResult result = s3Client.putObject( bucketName, keyName,
                new File( filePath ) );
        expEtag = result.getETag();
    }

    @Test(dataProvider = "rangeProvider")
    public void testGetObjectMetadata( String start, String end, int size )
            throws Exception {
        HttpHead request = new HttpHead(
                S3TestBase.s3ClientUrl + "/" + bucketName + "/" + keyName );
        request.setHeader( "Authorization",
                "Credential=" + accessKeys[ 0 ] + "/" );
        request.setHeader( "Range", "bytes=" + start + "-" + end );
        if ( !start.equals( "" ) && Integer.parseInt( start ) > fileSize ) {
            try {
                client = RestClient.createHttpClient();
                RestClient.sendRequest( client, request );
                Assert.fail( "exp fail but found success" );
            } catch ( Exception e ) {
                Assert.assertNotEquals( e.getMessage().indexOf( "errcode=416" ),
                        -1 );
            }
        } else {
            getRespAndcheckMetadata( request, size );
        }

        actSuccessTests.getAndIncrement();
    }

    @AfterClass
    private void tearDown() throws Exception {
        try {
            if ( actSuccessTests.get() == generatePageSize().length ) {
                UserUtils.deleteUser( userName );
                TestTools.LocalFile.removeFile( localPath );
            }
        } catch ( BaseException e ) {
            Assert.fail( "clean up failed:" + e.getMessage() );
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }

    private void getRespAndcheckMetadata( HttpHead request, int expSize )
            throws Exception {
        client = RestClient.createHttpClient();
        CloseableHttpResponse resp = RestClient.sendRequest( client, request );
        Assert.assertEquals( resp.getFirstHeader( "ETag" ).getValue(),
                "\"" + expEtag + "\"" );
        Assert.assertEquals( resp.getFirstHeader( "Content-Length" ).getValue(),
                String.valueOf( expSize ) );
    }
}
