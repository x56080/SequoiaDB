package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.ObjectMetadata;
import com.amazonaws.services.s3.model.PutObjectRequest;
import com.amazonaws.services.s3.model.PutObjectResult;
import com.sequoiadb.exception.BaseException;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import java.io.File;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * test content: withMetadata接口参数校验 testlink-case: seqDB-16479
 *
 * @author wangkexin
 * @Date 2019.01.07
 * @version 1.00
 */
public class TestWithMetadata16479 extends S3TestBase {
    private String bucketName = "bucket16479";
    private String keyName = "keyname16479";
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * 1024;
    private File localPath = null;
    private String filePath = null;
    private AtomicInteger actSuccessTests = new AtomicInteger( 0 );

    @DataProvider(name = "legalMetadataProvider")
    public Object[][] generateMetadata() {
        Map< String, String > expMeta = new HashMap<>();
        expMeta.put( "test1", "1234" );
        expMeta.put( "test2", "" );
        expMeta.put( "test3", null );

        Map< String, String > expMeta2 = new HashMap<>();
        expMeta2.put( "test", ObjectUtils.getRandomString( 2044 ) );

        return new Object[][] {
                // test a : 合法元数据信息，空串，null
                new Object[] { expMeta },
                // test b : 长度等于2kB （key+value总大小）
                new Object[] { expMeta2 } };
    }

    @BeforeClass
    private void setUp() throws Exception {
        localPath = new File( S3TestBase.workDir + File.separator
                + TestTools.getClassName() );
        filePath = localPath + File.separator + "localFile_" + fileSize
                + ".txt";
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath, fileSize );
        s3Client = CommLib.buildS3Client();
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );
    }

    @Test(dataProvider = "legalMetadataProvider")
    public void testLegalMetadata( Map< String, String > expMeta )
            throws Exception {
        PutObjectRequest request = new PutObjectRequest( bucketName, keyName,
                new File( filePath ) );
        ObjectMetadata metaData = new ObjectMetadata();
        metaData.setUserMetadata( expMeta );
        request.withMetadata( metaData );
        PutObjectResult result = s3Client.putObject( request );

        String actMd5 = result.getETag();
        String expMd5 = TestTools.getMD5( filePath );
        Assert.assertEquals( actMd5, expMd5,
                "md5 is wrong! the key name is : " + keyName );

        ObjectMetadata metadata = s3Client.getObjectMetadata( bucketName,
                keyName );
        Map< String, String > actMeta = new HashMap<>();
        actMeta = metadata.getUserMetadata();
        Assert.assertEquals( actMeta.size(), expMeta.size(), "expMeta is : "
                + printMap( expMeta ) + "actMeta is : " + printMap( actMeta ) );
        for ( Map.Entry< String, String > entry : expMeta.entrySet() ) {
            String expValue = entry.getValue() == null ? "" : entry.getValue();
            String actValue = actMeta.get( entry.getKey() ) == null ? ""
                    : actMeta.get( entry.getKey() );
            if ( !expValue.equals( actValue ) ) {
                Assert.fail( "Metadata is wrong ! exp : " + expValue
                        + ", but found : " + actValue );
            }
        }

        actSuccessTests.getAndIncrement();
    }

    @Test
    public void testIllegalMetadata() throws Exception {
        // 非法参数校验 长度超过2KB（key+value总大小）
        Map< String, String > expMeta = new HashMap<>();
        expMeta.put( "test", ObjectUtils.getRandomString( 2045 ) );

        PutObjectRequest request = new PutObjectRequest( bucketName, keyName,
                new File( filePath ) );
        ObjectMetadata metaData = new ObjectMetadata();
        metaData.setUserMetadata( expMeta );
        request.withMetadata( metaData );
        try {
            s3Client.putObject( request );
            Assert.fail( "when size more than 2KB , it should fail" );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorMessage(),
                    "Your metadata headers exceed the maximum allowed metadata size." );
        }
        actSuccessTests.getAndIncrement();
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( actSuccessTests.get() == ( generateMetadata().length + 1 ) ) {
                CommLib.deleteAllObjectVersions( s3Client, bucketName );
                s3Client.deleteBucket( bucketName );
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

    private String printMap( Map< String, String > map ) {
        String str = new String();
        for ( Map.Entry< String, String > entry : map.entrySet() ) {
            str += "Key = " + entry.getKey() + " value = " + entry.getValue()
                    + " ";
        }
        return str;
    }
}
