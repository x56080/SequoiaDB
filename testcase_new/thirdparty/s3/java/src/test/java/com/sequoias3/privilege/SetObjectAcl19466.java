package com.sequoias3.privilege;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AccessControlList;
import com.amazonaws.services.s3.model.CannedAccessControlList;
import com.amazonaws.services.s3.model.CanonicalGrantee;
import com.amazonaws.services.s3.model.CreateBucketRequest;
import com.amazonaws.services.s3.model.Grant;
import com.amazonaws.services.s3.model.GroupGrantee;
import com.amazonaws.services.s3.model.Owner;
import com.amazonaws.services.s3.model.Permission;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import com.sequoias3.testcommon.s3utils.PrivilegeUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.io.IOException;

/**
 * @Description seqDB-19466:指定versionId配置对象acl
 * @Author huangxiaoni
 * @Date 2019.09.24
 */

public class SetObjectAcl19466 extends S3TestBase {
    private boolean runSuccess = false;
    private String tcId = "19466";
    private AmazonS3 adminS3 = null;
    private String bucketName = "bucket" + tcId;
    private String keyName = "key" + tcId;
    private File localPath;
    private String fileContentA = "testA";
    private String fileContentB = "testB";

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator
                + TestTools.getClassName() );
        TestTools.LocalFile.removeFile( localPath );

        adminS3 = CommLib.buildS3Client();
        CommLib.clearBucket( adminS3, bucketName );
    }

    @Test
    private void test() throws Exception {
        // adminS3 create bucket and multi put object
        adminS3.createBucket( new CreateBucketRequest( bucketName ) );
        CommLib.setBucketVersioning( adminS3, bucketName, "Enabled" );
        adminS3.putObject( bucketName, keyName, fileContentA );
        adminS3.putObject( bucketName, keyName, fileContentB );

        // set object acl, current version
        String objCurVer = "1";
        adminS3.setObjectAcl( bucketName, keyName, objCurVer,
                CannedAccessControlList.AuthenticatedRead );
        // check results
        Grant[] expGrants1 = {
                new Grant(
                        new CanonicalGrantee(
                                adminS3.getS3AccountOwner().getId() ),
                        Permission.FullControl ),
                new Grant( GroupGrantee.AuthenticatedUsers, Permission.Read ) };
        checkObjectAcl( objCurVer, expGrants1 );
        checkObjectContent( adminS3, keyName, objCurVer, fileContentB );

        // set object acl, current version
        String objHisVer = "0";
        adminS3.setObjectAcl( bucketName, keyName, objHisVer,
                CannedAccessControlList.PublicReadWrite );
        // check results
        Grant[] expGrants2 = {
                new Grant(
                        new CanonicalGrantee(
                                adminS3.getS3AccountOwner().getId() ),
                        Permission.FullControl ),
                new Grant( GroupGrantee.AllUsers, Permission.Read ),
                new Grant( GroupGrantee.AllUsers, Permission.Write ) };
        checkObjectAcl( objHisVer, expGrants2 );
        checkObjectContent( adminS3, keyName, objHisVer, fileContentA );

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                CommLib.clearBucket( adminS3, bucketName );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            adminS3.shutdown();
        }
    }

    private void checkObjectAcl( String objectVersion, Grant[] grants ) {
        // check owner
        Owner owner = adminS3.getS3AccountOwner();
        AccessControlList acl = adminS3.getObjectAcl( bucketName, keyName,
                objectVersion );
        Assert.assertEquals( acl.getOwner(), owner );
        // check grant
        PrivilegeUtils.checkSetObjectAclResult( adminS3, bucketName, keyName,
                objectVersion, grants );
    }

    private void checkObjectContent( AmazonS3 authUserS3, String keyName,
            String objectVersion, String expFileContent ) throws Exception {
        String downfileMd5 = ObjectUtils.getMd5OfObject( authUserS3, localPath,
                bucketName, keyName, objectVersion );
        Assert.assertEquals( downfileMd5,
                TestTools.getMD5( expFileContent.getBytes() ) );
    }
}