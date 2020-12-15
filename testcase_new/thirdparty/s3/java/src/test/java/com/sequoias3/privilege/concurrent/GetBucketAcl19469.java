package com.sequoias3.privilege.concurrent;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CanonicalGrantee;
import com.amazonaws.services.s3.model.Grant;
import com.amazonaws.services.s3.model.Permission;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.PrivilegeUtils;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.IOException;

/**
 * @Description seqDB-19469:并发查询桶acl
 * @Author huangxiaoni
 * @Date 2019.09.17
 */
public class GetBucketAcl19469 extends S3TestBase {
    private boolean runSuccess = false;
    private int threadNum = 20;
    private String tcId = "19469";
    private AmazonS3 adminS3 = null;
    private String ownerId;
    private String bucketNameA = "bucket" + tcId + "a";
    private String bucketNameB = "bucket" + tcId + "b";
    private Grant grantA;
    private Grant grantB;

    @BeforeClass
    private void setUp() throws IOException {
        adminS3 = CommLib.buildS3Client();
        ownerId = adminS3.getS3AccountOwner().getId();
        CommLib.clearBucket( adminS3, bucketNameA );
        CommLib.clearBucket( adminS3, bucketNameB );

        adminS3.createBucket( bucketNameA );
        grantA = new Grant( new CanonicalGrantee( ownerId ), Permission.Read );
        PrivilegeUtils.setBucketAclByBody( adminS3, bucketNameA, grantA );

        adminS3.createBucket( bucketNameB );
        grantB = new Grant( new CanonicalGrantee( ownerId ), Permission.Write );
        PrivilegeUtils.setBucketAclByBody( adminS3, bucketNameB, grantB );
    }

    @Test
    private void test() throws Exception {
        // get bucket acl and check results in the thread
        ThreadExecutor threadExec = new ThreadExecutor();
        for ( int i = 0; i < threadNum; i++ ) {
            threadExec
                    .addWorker( new ThreadGetBucketAcl( bucketNameA, grantA ) );
            threadExec
                    .addWorker( new ThreadGetBucketAcl( bucketNameB, grantB ) );
        }
        threadExec.run();
        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                CommLib.clearBucket( adminS3, bucketNameA );
                CommLib.clearBucket( adminS3, bucketNameB );
            }
        } finally {
            adminS3.shutdown();
        }
    }

    private class ThreadGetBucketAcl {
        private String bucket;
        private Grant expGrant;

        public ThreadGetBucketAcl( String bucket, Grant expGrant ) {
            this.bucket = bucket;
            this.expGrant = expGrant;
        }

        @ExecuteOrder(step = 1)
        private void setBucketAcl() throws Exception {
            AmazonS3 s3 = null;
            try {
                s3 = CommLib.buildS3Client();
                PrivilegeUtils.checkSetBucketAclResult( s3, bucket, expGrant );
            } finally {
                if ( s3 != null ) {
                    s3.shutdown();
                }
            }
        }
    }
}
