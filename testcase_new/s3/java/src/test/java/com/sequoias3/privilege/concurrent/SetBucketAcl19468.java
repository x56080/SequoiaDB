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
import com.sequoias3.testcommon.s3utils.UserUtils;
import com.sequoias3.testcommon.s3utils.bean.UserCommDefind;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.IOException;

/**
 * @Description seqDB-19468:并发配置桶acl，权限不同
 * @Author huangxiaoni
 * @Date 2019.09.17
 */
public class SetBucketAcl19468 extends S3TestBase {
    private boolean runSuccess = false;
    private String tcId = "19468";
    private AmazonS3 adminS3 = null;
    private AmazonS3 userS3 = null;
    private String userName = "user" + tcId;
    private String userType = UserCommDefind.normal;
    private String[] userAcessKeys;
    private String userOwnerId;
    private String bucketName = "bucket" + tcId;

    @BeforeClass
    private void setUp() throws IOException {
        adminS3 = CommLib.buildS3Client();
        CommLib.clearUser( userName );
        CommLib.clearBucket( adminS3, bucketName );

        userAcessKeys = UserUtils.createUser( userName, userType );
        userS3 = CommLib
                .buildS3Client( userAcessKeys[ 0 ], userAcessKeys[ 1 ] );
        userOwnerId = userS3.getS3AccountOwner().getId();

        adminS3.createBucket( bucketName );
    }

    @Test
    private void test() throws Exception {
        ThreadExecutor threadExec = new ThreadExecutor();
        threadExec.addWorker( new ThreadSetBucketAcl( Permission.Read ) );
        threadExec.addWorker( new ThreadSetBucketAcl( Permission.Write ) );
        threadExec.run();

        // check results
        try {
            // multi thread concurrent, expect 1
            Grant grant = new Grant( new CanonicalGrantee( userOwnerId ),
                    Permission.Read );
            PrivilegeUtils.checkSetBucketAclResult( userS3, bucketName, grant );
        } catch ( AssertionError e ) {
            // or expect 2
            Grant grant = new Grant( new CanonicalGrantee( userOwnerId ),
                    Permission.Write );
            PrivilegeUtils.checkSetBucketAclResult( userS3, bucketName, grant );
        }

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                CommLib.clearBucket( adminS3, bucketName );
                CommLib.clearUser( userName );
            }
        } finally {
            adminS3.shutdown();
        }
    }

    private class ThreadSetBucketAcl {
        private Permission permission;

        private ThreadSetBucketAcl( Permission permission ) {
            this.permission = permission;
        }

        @ExecuteOrder(step = 1)
        private void setBucketAcl() throws Exception {
            AmazonS3 ownerS3 = null;
            AmazonS3 authS3 = null;
            try {
                ownerS3 = CommLib.buildS3Client();
                authS3 = CommLib.buildS3Client( userAcessKeys[ 0 ],
                        userAcessKeys[ 1 ] );
                // ownerS3 set object acl, authorized to userS3, and set
                // permission
                Grant grant = new Grant( new CanonicalGrantee(
                        authS3.getS3AccountOwner().getId() ), permission );
                PrivilegeUtils.setBucketAclByBody( ownerS3, bucketName, grant );
            } finally {
                if ( ownerS3 != null ) {
                    ownerS3.shutdown();
                }
                if ( authS3 != null ) {
                    authS3.shutdown();
                }
            }
        }
    }
}
