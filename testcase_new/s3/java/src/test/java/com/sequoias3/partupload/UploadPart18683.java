package com.sequoias3.partupload;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.PartETag;
import com.amazonaws.services.s3.model.UploadPartRequest;
import com.amazonaws.services.s3.model.UploadPartResult;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestRest;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.PartUploadUtils;
import com.sequoias3.testcommon.s3utils.RegionUtils;
import com.sequoias3.testcommon.s3utils.bean.UserCommDefind;
import org.springframework.http.HttpMethod;
import org.springframework.http.MediaType;
import org.springframework.web.client.HttpClientErrorException;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.io.IOException;
import java.net.URLEncoder;
import java.util.ArrayList;
import java.util.List;

/**
 * @Description seqDB-18683:上传分段号为降序
 * @Author wangkexin
 * @Date 2019.07.29
 */
@Test(groups = "partsizelimitoff")
public class UploadPart18683 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket18683";
    private String keyName = "key18683";
    private AmazonS3 s3Client = null;
    private long fileSize = 15 * 1024 * 1024;
    private long partSize = 3 * 1024 * 1024;
    private File localPath = null;
    private File file = null;
    private String filePath = null;
    private String uploadId = "";
    private List< PartETag > partEtags = new ArrayList<>();
    private MediaType type = MediaType
            .parseMediaType( "text/xml;charset=UTF-8" );

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

        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );
    }

    @Test
    private void testUpload() throws Exception {
        uploadId = PartUploadUtils.initPartUpload( s3Client, bucketName,
                keyName );
        // upload part 1
        uploadPart( 0, 1 );
        // upload part 5
        uploadPart( partSize * 4, 5 );
        // upload part 3
        uploadPart( partSize * 2, 3 );
        // 完成分段上传
        try {
            // 由于客户端发送请求时会自动将partEtags中的内容按照partNumber升序排序，所以此处使用REST接口发送请求
            completeMultipartUpload();
            Assert.fail(
                    "the list of parts was not in ascending order should fail." );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "InvalidPartOrder" );
        }
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

    private void uploadPart( long filepositon, int partNumber )
            throws IOException {
        UploadPartRequest partRequest = new UploadPartRequest().withFile( file )
                .withFileOffset( filepositon ).withPartNumber( partNumber )
                .withPartSize( partSize ).withBucketName( bucketName )
                .withKey( keyName ).withUploadId( uploadId );
        UploadPartResult uploadPartResult = s3Client.uploadPart( partRequest );
        partEtags.add( uploadPartResult.getPartETag() );
        String expPartMd5 = TestTools.getFilePartMD5( file, filepositon,
                partSize );
        String actPartMd5 = uploadPartResult.getPartETag().getETag();
        Assert.assertEquals( actPartMd5, expPartMd5, "part number = "
                + uploadPartResult.getPartETag().getPartNumber() );
    }

    private void completeMultipartUpload() throws Exception {
        TestRest rest = new TestRest( type );
        try {
            String body = "<CompleteMultipartUpload>";
            for ( PartETag etag : partEtags ) {
                body += "<Part><PartNumber>" + etag.getPartNumber()
                        + "</PartNumber><ETag>" + etag.getETag()
                        + "</ETag></Part>";
            }
            body += "</CompleteMultipartUpload>";

            rest.setApi( "/" + URLEncoder.encode( bucketName, "UTF-8" ) + "/"
                    + URLEncoder.encode( keyName, "UTF-8" ) + "?uploadId="
                    + uploadId )
                    .setRequestHeaders( UserCommDefind.authorization,
                            UserCommDefind.authValPre + S3TestBase.s3AccessKeyId
                                    + "/" )
                    .setRequestBody( body ).setRequestMethod( HttpMethod.POST )
                    .setResponseType( String.class ).exec();
        } catch ( HttpClientErrorException e ) {
            throw RegionUtils.httpToAmazon( e );
        }
    }
}
