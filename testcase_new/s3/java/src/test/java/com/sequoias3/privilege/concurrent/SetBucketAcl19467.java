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
 * @Description seqDB-19467:并发配置桶acl，权限相同
 * @Author huangxiaoni
 * @Date 2019.09.17
 */
public class SetBucketAcl19467 extends S3TestBase {
    private boolean runSuccess = false;
    private String tcId = "19467";
    private AmazonS3 adminS3 = null;
    private String ownerId;
    private String bucketName = "bucket" + tcId;

    @BeforeClass
    private void setUp() throws IOException {
        adminS3 = CommLib.buildS3Client();
        ownerId = adminS3.getS3AccountOwner().getId();
        CommLib.clearBucket( adminS3, bucketName );
        adminS3.createBucket( bucketName );
    }

    @Test
    private void test() throws Exception {
        ThreadExecutor threadExec = new ThreadExecutor();
        threadExec.addWorker( new ThreadSetBucketAclWithHeader() );
        threadExec.addWorker( new ThreadSetBucketAclWithBody() );
        threadExec.run();

        Grant grant = new Grant( new CanonicalGrantee( ownerId ),
                Permission.Read );
        PrivilegeUtils.checkSetBucketAclResult( adminS3, bucketName, grant );

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                CommLib.clearBucket( adminS3, bucketName );
            }
        } finally {
            adminS3.shutdown();
        }
    }

    private class ThreadSetBucketAclWithHeader {
        @ExecuteOrder(step = 1)
        private void setBucketAcl() throws Exception {
            Grant grant = new Grant( new CanonicalGrantee( ownerId ),
                    Permission.Read );
            PrivilegeUtils.setBucketAclByHeader( S3TestBase.s3AccessKeyId,
                    bucketName, grant );
        }
    }

    private class ThreadSetBucketAclWithBody {
        @ExecuteOrder(step = 1)
        private void setBucketAcl() throws Exception {
            AmazonS3 s3 = null;
            try {
                s3 = CommLib.buildS3Client();
                Grant grant = new Grant( new CanonicalGrantee( ownerId ),
                        Permission.Read );
                PrivilegeUtils.setBucketAclByBody( s3, bucketName, grant );
            } finally {
                if ( s3 != null ) {
                    s3.shutdown();
                }
            }
        }
    }
}
