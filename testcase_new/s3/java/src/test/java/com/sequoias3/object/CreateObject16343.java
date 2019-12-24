package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.GetObjectRequest;
import com.amazonaws.services.s3.model.ObjectMetadata;
import com.amazonaws.services.s3.model.PutObjectResult;
import com.amazonaws.services.s3.model.S3Object;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.io.IOException;
import java.util.Date;

/**
 * test content: 开启版本控制增加对象，禁用版本控制后上传同名对象 testlink-case: seqDB-16343
 *
 * @author wangkexin
 * @Date 2018.11.9
 * @version 1.00
 */
public class CreateObject16343 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket16343";
    private String keyName = "/aa/bb/object16343.png";
    private AmazonS3 s3Client = null;
    private File localPath = null;
    private String firstTime_expContent = null;
    private String secondTime_expContent = null;

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator + TestTools
                .getClassName() );
        s3Client = CommLib.buildS3Client();
        // create bucket and set bucket version status
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );
        CommLib.setBucketVersioning( s3Client, bucketName, "Enabled" );
    }

    @Test
    public void testPutObject() throws Exception {
        // put the same object with contents.
        firstTime_expContent = "first_time_file16343";
        PutObjectResult putObjResult = s3Client
                .putObject( bucketName, keyName, firstTime_expContent );
        Date expFirstCreateTime = new Date();
        String historyVersionId = putObjResult.getVersionId();

        CommLib.setBucketVersioning( s3Client, bucketName, "Suspended" );

        secondTime_expContent = "second_time_file16343";
        s3Client.putObject( bucketName, keyName, secondTime_expContent );
        Date expSecondCreateTime = new Date();

        // check result
        S3Object currObject = s3Client.getObject( bucketName, keyName );
        S3Object hisObject = s3Client.getObject(
                new GetObjectRequest( bucketName, keyName, historyVersionId ) );

        checkObjectResult( hisObject, historyVersionId, expFirstCreateTime,
                firstTime_expContent );
        checkObjectResult( currObject, null, expSecondCreateTime,
                secondTime_expContent );

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        if ( runSuccess ) {
            CommLib.deleteAllObjectVersions( s3Client, bucketName );
            s3Client.deleteBucket( bucketName );
            TestTools.LocalFile.removeFile( localPath );
        }
    }

    private void checkObjectResult( S3Object object, String versionid,
            Date expDate, String expContent ) throws Exception {
        // check create time
        ObjectMetadata metadata = object.getObjectMetadata();
        Date actCreateDate = metadata.getLastModified();
        if ( actCreateDate.after( expDate ) ) {
            Assert.fail(
                    "object create time is different! versionid is " + metadata
                            .getVersionId() + ",the actCreateDate is : "
                            + actCreateDate.toString() + ",the expDate is : "
                            + expDate.toString() );
        }

        // check object content by md5
        String actMd5 = ObjectUtils
                .getMd5OfObject( s3Client, localPath, bucketName, keyName,
                        versionid );
        String expMd5 = TestTools.getMD5( expContent.getBytes() );
        Assert.assertEquals( actMd5, expMd5,
                "The md5 value of the current version is different." );
    }
}
