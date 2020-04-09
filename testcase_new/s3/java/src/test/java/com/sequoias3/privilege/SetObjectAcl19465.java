package com.sequoias3.privilege;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AccessControlList;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.CannedAccessControlList;
import com.amazonaws.services.s3.model.CanonicalGrantee;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.Grant;
import com.amazonaws.services.s3.model.Owner;
import com.amazonaws.services.s3.model.Permission;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.s3utils.PrivilegeUtils;
import com.sequoias3.testcommon.s3utils.UserUtils;
import com.sequoias3.testcommon.s3utils.bean.UserCommDefind;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.IOException;

/**
 * @Description seqDB-19465:无对象访问权限的用户配置对象acl
 * @Author huangxiaoni
 * @Date 2019.09.24
 */

public class SetObjectAcl19465 extends S3TestBase {
    private boolean runSuccess = false;
    private String tcId = "19465";
    private AmazonS3 adminS3 = null;
    private AmazonS3 userS3 = null;
    private String userName = "user" + tcId;
    private String userType = UserCommDefind.normal;
    private String bucketName = "bucket" + tcId;
    private String keyName = "key" + tcId;
    private String fileContent = "test";

    @BeforeClass
    private void setUp() throws IOException {
        adminS3 = CommLib.buildS3Client();
        CommLib.clearBucket( adminS3, bucketName );
        CommLib.clearUser( userName );
        String[] acessKeys = UserUtils.createUser( userName, userType );
        userS3 = CommLib.buildS3Client( acessKeys[ 0 ], acessKeys[ 1 ] );
    }

    @Test
    private void test() throws Exception {
        adminS3.createBucket( new CreateBucketRequest( bucketName ) );
        adminS3.putObject( bucketName, keyName, fileContent );

        AccessControlList acl = adminS3.getObjectAcl( bucketName, keyName );
        // check owner
        Owner owner = adminS3.getS3AccountOwner();
        Assert.assertEquals( acl.getOwner(), owner );
        // check grant
        Grant grant = new Grant( new CanonicalGrantee( owner.getId() ),
                Permission.FullControl );
        PrivilegeUtils.checkSetObjectAclResult( adminS3, bucketName, keyName,
                grant );

        // not object owner set object's acl
        try {
            userS3.setObjectAcl( bucketName, keyName,
                    CannedAccessControlList.PublicRead );
            Assert.fail( "expect fail but success." );
        } catch ( AmazonS3Exception e ) {
            Assert.assertEquals( e.getErrorCode(), "AccessDenied" );
        }

        // check object acl again
        PrivilegeUtils.checkSetObjectAclResult( adminS3, bucketName, keyName,
                grant );

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
}