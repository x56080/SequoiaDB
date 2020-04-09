package com.sequoias3.head;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.GetObjectMetadataRequest;
import com.amazonaws.services.s3.model.ObjectMetadata;
import com.amazonaws.services.s3.model.PutObjectResult;
import com.sequoiadb.exception.BaseException;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.HeadUtils;
import com.sequoias3.testcommon.s3utils.UserUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.Date;

/**
 * test content: 指定versionId查询对象 testlink-case: seqDB-16681
 *
 * @author wangkexin
 * @Date 2018.12.07
 * @version 1.00
 */

public class TestGetObjectMetadata16681 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket16681";
    private String userName = "user16681";
    private String roleName = "normal";
    private String keyName = "key16681";
    private String content = "content16681";
    private AmazonS3 s3Client = null;

    @BeforeClass
    private void setUp() throws Exception {
        CommLib.clearUser( userName );
        String[] accessKeys = UserUtils.createUser( userName, roleName );
        s3Client = CommLib.buildS3Client( accessKeys[ 0 ], accessKeys[ 1 ] );
    }

    @Test
    private void testGetObjectMetadata() throws Exception {
        s3Client.createBucket( bucketName );
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );
        s3Client.putObject( bucketName, keyName, content );
        s3Client.putObject( bucketName, keyName, content );
        Date date1 = new Date();
        PutObjectResult result = s3Client.putObject( bucketName, keyName,
                content );
        Date date2 = new Date();
        String expEtag = result.getETag();
        String expVersionid = result.getVersionId();

        ObjectMetadata metadata = s3Client
                .getObjectMetadata( new GetObjectMetadataRequest( bucketName,
                        keyName, expVersionid ) );

        // check
        Assert.assertEquals( metadata.getETag(), expEtag, "etag is wrong!" );
        Assert.assertEquals( metadata.getContentLength(),
                ( long ) content.length(), "size is wrong!" );
        Assert.assertEquals( metadata.getVersionId(), expVersionid,
                "versionid is wrong!" );

        Date actDate = metadata.getLastModified();
        // 校验对象lastModified时间在[date1, date2]范围内，只精确到秒，忽略毫秒
        if ( actDate.getTime() < ( date1.getTime() / 1000 ) * 1000
                || actDate.getTime() > ( date2.getTime() / 1000 ) * 1000 ) {
            Assert.fail( "lastmodified is wrong!  actDate is : "
                    + HeadUtils.getGMTDate( actDate ) + ", date1 is :"
                    + HeadUtils.getGMTDate( date1 ) + ", date2 is : "
                    + HeadUtils.getGMTDate( date2 ) );
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
}
