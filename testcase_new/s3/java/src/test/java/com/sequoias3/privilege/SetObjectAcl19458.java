package com.sequoias3.privilege;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CanonicalGrantee;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.Grant;
import com.amazonaws.services.s3.model.Permission;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import com.sequoias3.testcommon.s3utils.PrivilegeUtils;
import com.sequoias3.testcommon.s3utils.UserUtils;
import com.sequoias3.user.UserCommDefind;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.io.IOException;

/**
 * @Description seqDB-19458:配置相同桶中不同对象的对象acl，其中被授权人为不同用户
 * @Author huangxiaoni
 * @Date 2019.09.24
 */

public class SetObjectAcl19458 extends S3TestBase {
    private boolean runSuccess = false;
    private String tcId = "19458";
    private AmazonS3 adminS3 = null;
    private AmazonS3 userS3A = null;
    private AmazonS3 userS3B = null;
    private String userNameA = "user" + tcId + "A";
    private String userNameB = "user" + tcId + "B";
    private String userType = UserCommDefind.normal;
    private String bucketName = "bucket" + tcId;
    private String keyNameA = "key" + tcId + "A";
    private String keyNameB = "key" + tcId + "B";
    private File localPath;
    private String fileContentA = "testA";
    private String fileContentB = "testB";

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator + TestTools
                .getClassName() );
        TestTools.LocalFile.removeFile( localPath );

        adminS3 = CommLib.buildS3Client();
        CommLib.clearUser( userNameA );
        CommLib.clearUser( userNameB );

        String[] acessKeysA = UserUtils.createUser( userNameA, userType );
        userS3A = CommLib.buildS3Client( acessKeysA[ 0 ], acessKeysA[ 1 ] );

        String[] acessKeysB = UserUtils.createUser( userNameB, userType );
        userS3B = CommLib.buildS3Client( acessKeysB[ 0 ], acessKeysB[ 1 ] );
    }

    @Test
    private void test() throws Exception {
        // adminS3 create bucket and object
        adminS3.createBucket( new CreateBucketRequest( bucketName ) );
        adminS3.putObject( bucketName, keyNameA, fileContentA );
        adminS3.putObject( bucketName, keyNameB, fileContentB );

        // set object acl, authorized to userS3A or userS3B, and check results
        setObjectAclAndCheckResults( userS3A, keyNameA, Permission.Read );
        setObjectAclAndCheckResults( userS3B, keyNameB, Permission.Write );

        // userS3A or userS3B get object A and B
        checkObjectContent( userS3A, keyNameA, fileContentA );
        checkObjectContent( userS3A, keyNameB, fileContentB );

        checkObjectContent( userS3B, keyNameA, fileContentA );
        checkObjectContent( userS3B, keyNameB, fileContentB );

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                CommLib.clearBucket( adminS3, bucketName );
                CommLib.clearUser( userNameA );
                CommLib.clearUser( userNameB );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            adminS3.shutdown();
            userS3A.shutdown();
            userS3B.shutdown();
        }
    }

    private void setObjectAclAndCheckResults( AmazonS3 authUserS3,
            String keyName, Permission permission ) {
        // adminS3 set object acl, authorized to userS3
        Grant grant = new Grant(
                new CanonicalGrantee( authUserS3.getS3AccountOwner().getId() ),
                permission );
        PrivilegeUtils
                .setObjectAclByBody( adminS3, bucketName, keyName, grant );
        // userS3 get object acl and check results
        PrivilegeUtils.checkSetObjectAclResult( authUserS3, bucketName, keyName,
                grant );
    }

    private void checkObjectContent( AmazonS3 authUserS3, String keyName,
            String expFileContent ) throws Exception {
        String downfileMd5 = ObjectUtils
                .getMd5OfObject( authUserS3, localPath, bucketName, keyName );
        Assert.assertEquals( downfileMd5,
                TestTools.getMD5( expFileContent.getBytes() ) );
    }
}