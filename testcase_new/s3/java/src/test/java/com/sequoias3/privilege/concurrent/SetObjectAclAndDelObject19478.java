package com.sequoias3.privilege.concurrent;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.CanonicalGrantee;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.Grant;
import com.amazonaws.services.s3.model.GroupGrantee;
import com.amazonaws.services.s3.model.Permission;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.PrivilegeUtils;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.io.IOException;

/**
 * @Description seqDB-19478: 并发配置对象acl和删除对象
 * @Author wangkexin
 * @Date 2019.09.24
 */
public class SetObjectAclAndDelObject19478 extends S3TestBase {
    private String bucketName = "bucket19478";
    private String keyName = "key19478";
    private AmazonS3 s3Client = null;
    private long fileSize = 100 * 1024;
    private File localPath = null;
    private File file = null;
    private String filePath = null;
    private boolean runSuccess = false;
    private String ownerId;

    @BeforeClass
    private void setUp() throws IOException {
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
        ownerId = s3Client.getS3AccountOwner().getId();
        s3Client.putObject( bucketName, keyName, file );
    }

    @Test
    private void testSetObjectAcl() throws Exception {
        Grant[] expGrant = { new Grant( new CanonicalGrantee( ownerId ),
                Permission.ReadAcp ),
                new Grant( GroupGrantee.AllUsers, Permission.Read ) };

        ThreadExecutor threadExec = new ThreadExecutor();
        threadExec.addWorker( new ThreadSetObjectAcl( expGrant ) );
        threadExec.addWorker( new ThreadDeleteObject() );
        threadExec.run();

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

    private class ThreadSetObjectAcl {
        private AmazonS3 s3 = CommLib.buildS3Client();
        private Grant[] grant;

        public ThreadSetObjectAcl( Grant[] grant ) {
            this.grant = grant;
        }

        @ExecuteOrder(step = 1)
        private void setObjectAcl() {
            try {
                PrivilegeUtils
                        .setObjectAclByBody( s3, bucketName, keyName, grant );
            } catch ( AmazonS3Exception e ) {
                if ( !e.getErrorCode().equals( "NoSuchKey" ) ) {
                    throw e;
                }
            } finally {
                if ( s3 != null ) {
                    s3.shutdown();
                }
            }
        }
    }

    private class ThreadDeleteObject {
        private AmazonS3 s3 = CommLib.buildS3Client();

        @ExecuteOrder(step = 1)
        private void deleteObject() {
            try {
                s3.deleteObject( bucketName, keyName );
            } finally {
                if ( s3 != null ) {
                    s3.shutdown();
                }
            }
        }
    }
}
