package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
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
 * @Description seqDB-16338: create object,the key name of the same name already
 *              exists
 * @author wuyan
 * @Date 2018.11.12
 * @version 1.00
 */
public class UpdateObject16338 extends S3TestBase {
    private boolean runSuccess = false;
    private String keyName = "aa%bb%object16338";
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * 50;
    private int updateSize = 1024 * 200;
    private File localPath = null;
    private String filePath = null;
    private String updatePath = null;

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator
                + TestTools.getClassName() );
        filePath = localPath + File.separator + "localFile_" + fileSize
                + ".txt";
        updatePath = localPath + File.separator + "localFile_" + updateSize
                + ".txt";

        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath, fileSize );
        TestTools.LocalFile.createFile( updatePath, updateSize );
        s3Client = CommLib.buildS3Client();
    }

    @Test
    public void testCreateObject() throws Exception {
        s3Client.putObject( S3TestBase.bucketName, keyName,
                new File( filePath ) );
        updateObjectWithSameContent( S3TestBase.bucketName );
        updateObjectWithDiffContent( S3TestBase.bucketName );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                s3Client.deleteObject( S3TestBase.bucketName, keyName );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            s3Client.shutdown();
        }
    }

    private void updateObjectWithSameContent( String bucketName )
            throws Exception {
        // get the create object date
        S3Object object = s3Client.getObject( bucketName, keyName );
        Date createDate = object.getObjectMetadata().getLastModified();

        PutObjectResult result = s3Client.putObject( bucketName, keyName,
                new File( filePath ) );
        // check the versionId, should be null
        Assert.assertEquals( result.getVersionId(), null );

        // check the modify date
        S3Object updateObject = s3Client.getObject( bucketName, keyName );
        Date updateDate = updateObject.getObjectMetadata().getLastModified();
        if ( updateDate.getTime() < createDate.getTime() ) {
            Assert.fail(
                    "updateDate must be grater than createDate! updateDate:"
                            + updateDate.getTime() + "\t createDate:"
                            + createDate.getTime() );
        }

        // check the context
        String downfileMd5 = ObjectUtils.getMd5OfObject( s3Client, localPath,
                bucketName, keyName );
        Assert.assertEquals( downfileMd5, TestTools.getMD5( filePath ) );
    }

    private void updateObjectWithDiffContent( String bucketName )
            throws Exception {

        PutObjectResult result = s3Client.putObject( bucketName, keyName,
                new File( updatePath ) );
        // check the versionId, should be null
        Assert.assertEquals( result.getVersionId(), null );

        // check the content
        String downfileMd5 = ObjectUtils.getMd5OfObject( s3Client, localPath,
                bucketName, keyName );

        Assert.assertEquals( downfileMd5, TestTools.getMD5( updatePath ) );
    }

}
