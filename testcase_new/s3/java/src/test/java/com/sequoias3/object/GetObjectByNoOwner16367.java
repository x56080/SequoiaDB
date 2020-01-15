package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.BucketVersioningConfiguration;
import com.amazonaws.services.s3.model.PutObjectRequest;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.UserUtils;
import com.sequoias3.testcommon.s3utils.bean.UserCommDefind;
import org.json.JSONObject;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.io.IOException;

/**
 * @Description: seqDB-16367 ::非桶管理用户获取对象
 * @author fanyu
 * @Date:2018年11月12日
 * @version:1.0
 */
public class GetObjectByNoOwner16367 extends S3TestBase {
    private boolean runSuccess = false;
    private String bucketName = "bucket16367";
    private String username = "user16367";
    private String objectName = "object16367";
    private AmazonS3 s3Client = null;
    private String accessKeyID = null;
    private String secretAccessKey = null;
    private int fileSize = 1024 * 200;
    private File localPath = null;
    private String filePath = null;

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator + TestTools
                .getClassName() );
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        filePath =
                localPath + File.separator + "localFile_" + fileSize + ".txt";
        TestTools.LocalFile.createFile( filePath, fileSize );
        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
        CommLib.setBucketVersioning( s3Client, bucketName,
                BucketVersioningConfiguration.ENABLED );
    }

    @Test
    private void test() throws Exception {
        s3Client.putObject( new PutObjectRequest( bucketName, objectName,
                new File( filePath ) ) );
        createUser();
        getObjectByNoOwner();
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                CommLib.clearBucket( s3Client, bucketName );
                UserUtils.deleteUser( username, UserUtils.accessKeyId, true );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            if ( s3Client != null ) {
                s3Client.shutdown();
            }
        }
    }

    private void getObjectByNoOwner() {
        // get object by no owner
        AmazonS3 s3 = null;
        try {
            s3 = CommLib.buildS3Client( accessKeyID, secretAccessKey );
            s3.getObject( bucketName, objectName );
            Assert.fail( "exp fail but act success" );
        } catch ( AmazonS3Exception e ) {
            if ( e.getStatusCode() != 403 ) {
                Assert.fail( e.getMessage() );
            }
        } finally {
            if ( s3 != null ) {
                s3.shutdown();
            }
        }
    }

    private void createUser() {
        JSONObject user = UserUtils.createUser( username, UserCommDefind.admin,
                UserUtils.accessKeyId );
        JSONObject userJSON = user.getJSONObject( UserCommDefind.accessKeys );
        accessKeyID = userJSON.getString( UserCommDefind.accessKeyID );
        secretAccessKey = userJSON.getString( UserCommDefind.secretAccessKey );
    }
}
