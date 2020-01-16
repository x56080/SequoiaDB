package com.sequoias3.privilege.concurrent;

import java.io.IOException;

import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

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
import com.sequoias3.testcommon.s3utils.PrivilegeUtils;

/**
 * @Description seqDB-19474: 并发配置桶acl和删除桶
 * @Author wangkexin
 * @Date 2019.09.24
 */
public class SetBucketAclAndDelBucket19474 extends S3TestBase {
    private int bucketNum = 20;
    private String bucketNameBase = "bucket19474-";
    private AmazonS3 s3Client = null;
    private String ownerId;

    @BeforeClass
    private void setUp() throws IOException {
        s3Client = CommLib.buildS3Client();
        for ( int i = 0; i < bucketNum; i++ ) {
            CommLib.clearBucket( s3Client, bucketNameBase + i );
            s3Client.createBucket( new CreateBucketRequest( bucketNameBase + i
            ) );
        }
        ownerId = s3Client.getS3AccountOwner().getId();
    }

    @Test
    private void testSetObjectAcl() throws Exception {
        Grant[] expGrant = { new Grant( new CanonicalGrantee( ownerId ),
                Permission.ReadAcp ),
                new Grant( GroupGrantee.AllUsers, Permission.Read ) };

        ThreadExecutor threadExec = new ThreadExecutor();
        for ( int i = 0; i < bucketNum; i++ ) {
            threadExec.addWorker( new ThreadSetBucketAcl( expGrant,
                    bucketNameBase + i ) );
            threadExec
                    .addWorker( new ThreadDeleteBucket( bucketNameBase + i ) );
        }
        threadExec.run();
    }

    @AfterClass
    private void tearDown() {
        s3Client.shutdown();
    }

    private class ThreadSetBucketAcl {
        private AmazonS3 s3 = CommLib.buildS3Client();
        private Grant[] grant;
        private String bucketName;

        public ThreadSetBucketAcl( Grant[] grant, String bucketName ) {
            this.grant = grant;
            this.bucketName = bucketName;
        }

        @ExecuteOrder(step = 1)
        private void setObjectAcl() {
            try {
                PrivilegeUtils.setBucketAclByBody( s3, bucketName, grant );
            } catch ( AmazonS3Exception e ) {
                if ( !e.getErrorCode().equals( "NoSuchBucket" ) ) {
                    throw e;
                }
            } finally {
                if ( s3 != null ) {
                    s3.shutdown();
                }
            }
        }
    }

    private class ThreadDeleteBucket {
        private AmazonS3 s3 = CommLib.buildS3Client();
        private String bucketName;

        public ThreadDeleteBucket( String bucketName ) {
            this.bucketName = bucketName;
        }

        @ExecuteOrder(step = 1)
        private void deleteBucket() throws InterruptedException {
            try {
                s3.deleteBucket( bucketName );
            } finally {
                if ( s3 != null ) {
                    s3.shutdown();
                }
            }
        }
    }
}
