package com.sequoias3.privilege;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CanonicalGrantee;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.Grant;
import com.amazonaws.services.s3.model.Permission;
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
 * @Description seqDB-19447:为相同用户多次赋予桶访问权限
 * @Author huangxiaoni
 * @Date 2019.09.24
 */

public class SetBucketAcl19447 extends S3TestBase {
    private boolean runSuccess = false;
    private String tcId = "19447";
    private AmazonS3 adminS3 = null;
    private AmazonS3 userS3 = null;
    private String userName = "user" + tcId;
    private String userType = UserCommDefind.normal;
    private String bucketName = "bucket" + tcId;

    @BeforeClass
    private void setUp() throws IOException {
        adminS3 = CommLib.buildS3Client();
        CommLib.clearBucket( adminS3, bucketName );
        CommLib.clearUser( userName );
        String[] acessKeys = UserUtils.createUser( userName, userType );
        userS3 = CommLib.buildS3Client( acessKeys[ 0 ], acessKeys[ 1 ] );
    }

    /*
     * adminS3 create bucket, and set bucket acl, then authorized to userS3
     */
    @Test
    private void test() throws Exception {
        adminS3.createBucket( new CreateBucketRequest( bucketName ) );
        // set bucket acl
        setBucketAclAndCheckResults( Permission.FullControl );
        // set bucket acl again, same Permission
        setBucketAclAndCheckResults( Permission.FullControl );
        // set bucket acl again, different Permission
        setBucketAclAndCheckResults( Permission.Read );

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
            userS3.shutdown();
        }
    }

    private void setBucketAclAndCheckResults( Permission permission ) {
        // adminS3 set bucket acl, authorized to userS3
        Grant grant = new Grant(
                new CanonicalGrantee( userS3.getS3AccountOwner().getId() ),
                permission );
        PrivilegeUtils.setBucketAclByBody( adminS3, bucketName, grant );
        // userS3 get bucket acl and check results
        PrivilegeUtils.checkSetBucketAclResult( userS3, bucketName, grant );
    }
}
