package com.sequoias3.object;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.GetObjectRequest;
import com.amazonaws.services.s3.model.ObjectMetadata;
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
 * @Description seqDB-16378: enabling bucket versioning,get object with
 *              conditions: match ifModifiedSince and mismatch ifUnModifiedSince
 * @author wuyan
 * @Date 2018.11.14
 * @version 1.00
 */
public class GetObjectWithMatchConditions16378 extends S3TestBase {
    private boolean runSuccess = false;
    private String key = "/aa/bb/object16378";
    private AmazonS3 s3Client = null;
    private int fileSize = 1024 * 100;
    private int updateSize = 1024 * 150;
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
        ObjectUtils.deleteObjectAllVersions( s3Client,
                S3TestBase.enableVerBucketName, key );
    }

    @Test
    public void testGetObject() throws Exception {
        s3Client.putObject( S3TestBase.enableVerBucketName, key,
                new File( filePath ) );
        Date createDate = getCreateDate( S3TestBase.enableVerBucketName );
        s3Client.putObject( S3TestBase.enableVerBucketName, key,
                new File( updatePath ) );

        // set date one day less than create time
        long timestamp = createDate.getTime() - 96784000l;
        Date unModifydate = new Date( timestamp );
        String curVersionId = "1";
        GetObjectRequest request = new GetObjectRequest(
                S3TestBase.enableVerBucketName, key, curVersionId );
        request.withUnmodifiedSinceConstraint( unModifydate )
                .withModifiedSinceConstraint( createDate );
        S3Object object = s3Client.getObject( request );

        Assert.assertNull( object, "does not match object!" );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                ObjectUtils.deleteObjectAllVersions( s3Client,
                        S3TestBase.enableVerBucketName, key );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            s3Client.shutdown();
        }
    }

    private Date getCreateDate( String bucketName ) {
        S3Object object = s3Client.getObject( bucketName, key );
        ObjectMetadata metadata = object.getObjectMetadata();
        Date date = metadata.getLastModified();
        return date;
    }
}
