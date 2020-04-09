package com.sequoias3.partupload;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.ListPartsRequest;
import com.amazonaws.services.s3.model.PartETag;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import com.sequoias3.testcommon.s3utils.PartUploadUtils;
import com.sequoias3.testcommon.s3utils.UserUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.io.IOException;
import java.util.List;

/**
 * @Description seqDB-18731:非桶管理用户查询分段列表
 * @Author wangkexin
 * @Date 2019.08.05
 */

public class ListParts18731 extends S3TestBase {
    private boolean runSuccess = false;
    private String userNameA = "userA18731";
    private String userNameB = "userB18731";
    private String roleName = "normal";
    private String bucketName = "bucket18731";
    private String keyName = "key18731";
    private AmazonS3 s3ClientA = null;
    private AmazonS3 s3ClientB = null;
    private long fileSize = 10 * 1024 * 1024;
    private File localPath = null;
    private File file = null;
    private String filePath = null;

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

        CommLib.clearUser( userNameA );
        CommLib.clearUser( userNameB );

        // 创建用户 A
        String[] acessKeys = UserUtils.createUser( userNameA, roleName );
        s3ClientA = CommLib.buildS3Client( acessKeys[ 0 ], acessKeys[ 1 ] );

        CommLib.clearBucket( s3ClientA, bucketName );
        s3ClientA.createBucket( new CreateBucketRequest( bucketName ) );

        // 创建用户 B
        String[] acessKeysB = UserUtils.createUser( userNameB, roleName );
        s3ClientB = CommLib.buildS3Client( acessKeysB[ 0 ], acessKeysB[ 1 ] );
    }

    @Test
    private void testListParts() throws Exception {
        // 用户A分段上传对象
        String uploadId = PartUploadUtils.initPartUpload( s3ClientA, bucketName,
                keyName );
        List< PartETag > partEtags = PartUploadUtils.partUpload( s3ClientA,
                bucketName, keyName, uploadId, file );

        // 用户B执行查询分段列表
        ListPartsRequest request = new ListPartsRequest( bucketName, keyName,
                uploadId );
        try {
            s3ClientB.listParts( request );
            Assert.fail( "list parts by other user should fail." );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "AccessDenied" );
        }

        PartUploadUtils.completeMultipartUpload( s3ClientA, bucketName, keyName,
                uploadId, partEtags );

        // 检查结果
        String expMd5 = TestTools.getMD5( filePath );
        String downloadMd5 = ObjectUtils.getMd5OfObject( s3ClientA, localPath,
                bucketName, keyName );
        Assert.assertEquals( downloadMd5, expMd5 );
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                CommLib.clearBucket( s3ClientA, bucketName );
                UserUtils.deleteUser( userNameA );
                UserUtils.deleteUser( userNameB );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            if ( s3ClientA != null ) {
                s3ClientA.shutdown();
            }
            if ( s3ClientB != null ) {
                s3ClientB.shutdown();
            }
        }
    }
}
