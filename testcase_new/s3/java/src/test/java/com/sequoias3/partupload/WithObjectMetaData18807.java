package com.sequoias3.partupload;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.InitiateMultipartUploadRequest;
import com.amazonaws.services.s3.model.InitiateMultipartUploadResult;
import com.amazonaws.services.s3.model.ObjectMetadata;
import com.amazonaws.services.s3.model.PartETag;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import com.sequoias3.testcommon.s3utils.PartUploadUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import java.io.File;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * test content:withMetadata接口参数校验 testlink-case: seqDB-18807
 *
 * @author wangkexin
 * @Date 2019.8.6
 * @version 1.00
 */
public class WithObjectMetaData18807 extends S3TestBase {
    private String bucketName = "bucket18807";
    private String keyName = "key18807";
    private long fileSize = 5 * 1024;
    private File localPath = null;
    private File file = null;
    private String filePath = null;
    private AmazonS3 s3Client = null;
    private AtomicInteger actSuccessTests = new AtomicInteger( 0 );

    @DataProvider(name = "legalMetadataProvider")
    public Object[][] generateMetadata() {
        Map<String, String> expMeta = new HashMap<>();
        expMeta.put( "test1", "1234" );
        expMeta.put( "test2", "" );
        expMeta.put( "test3", null );

        Map<String, String> expMeta2 = new HashMap<>();
        expMeta2.put( "test", ObjectUtils.getRandomString( 2044 ) );

        return new Object[][] {
                // test a : 合法元数据信息，空串，null
                new Object[] { expMeta },
                // test b : 长度等于2kB （key+value总大小）
                new Object[] { expMeta2 } };
    }

    @BeforeClass
    private void setUp() throws Exception {
        localPath = new File( S3TestBase.workDir + File.separator + TestTools
                .getClassName() );
        filePath =
                localPath + File.separator + "localFile_" + fileSize + ".txt";

        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath, fileSize );
        file = new File( filePath );

        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );
    }

    @Test(dataProvider = "legalMetadataProvider")
    public void testLegalMetaData( Map<String, String> meta ) throws Exception {
        InitiateMultipartUploadRequest initRequest = new InitiateMultipartUploadRequest(
                bucketName, keyName );
        ObjectMetadata userMetadata = new ObjectMetadata();
        userMetadata.setUserMetadata( meta );
        initRequest.withObjectMetadata( userMetadata );
        InitiateMultipartUploadResult result = s3Client
                .initiateMultipartUpload( initRequest );
        String uploadId = result.getUploadId();
        Assert.assertNotEquals( uploadId, null );
        List<PartETag> partEtags = PartUploadUtils
                .partUpload( s3Client, bucketName, keyName, uploadId, file );
        PartUploadUtils.completeMultipartUpload( s3Client, bucketName, keyName,
                uploadId, partEtags );

        ObjectMetadata metadata = s3Client
                .getObjectMetadata( bucketName, keyName );
        Map<String, String> actMeta = new HashMap<>();
        actMeta = metadata.getUserMetadata();
        Assert.assertEquals( actMeta.size(), meta.size(),
                "expMeta is : " + printMap( meta ) + "actMeta is : " + printMap(
                        actMeta ) );
        for ( Map.Entry<String, String> entry : meta.entrySet() ) {
            String expValue = entry.getValue() == null ? "" : entry.getValue();
            String actValue = actMeta.get( entry.getKey() ) == null ?
                    "" :
                    actMeta.get( entry.getKey() );
            if ( !expValue.equals( actValue ) ) {
                Assert.fail( "Metadata is wrong ! exp : " + expValue
                        + ", but found : " + actValue );
            }
        }
        actSuccessTests.getAndIncrement();
    }

    @Test
    public void testIllegalMetaData() throws Exception {
        InitiateMultipartUploadRequest initRequest = new InitiateMultipartUploadRequest(
                bucketName, keyName );
        ObjectMetadata metadata = new ObjectMetadata();
        Map<String, String> meta = new HashMap<>();
        meta.put( "test", ObjectUtils.getRandomString( 2045 ) );
        metadata.setUserMetadata( meta );
        initRequest.withObjectMetadata( metadata );
        try {
            s3Client.initiateMultipartUpload( initRequest );
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
            if ( actSuccessTests.get() == generateMetadata().length + 1 ) {
                CommLib.clearBucket( s3Client, bucketName );
            }
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }

    private String printMap( Map<String, String> map ) {
        String str = new String();
        for ( Map.Entry<String, String> entry : map.entrySet() ) {
            str += "Key = " + entry.getKey() + " value = " + entry.getValue()
                    + " ";
        }
        return str;
    }
}
