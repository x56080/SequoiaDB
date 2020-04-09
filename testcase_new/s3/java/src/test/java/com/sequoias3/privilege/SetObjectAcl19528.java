package com.sequoias3.privilege;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.CanonicalGrantee;
import com.amazonaws.services.s3.model.CopyObjectRequest;
import com.amazonaws.services.s3.model.Grant;
import com.amazonaws.services.s3.model.GroupGrantee;
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
 * @Description seqDB-19528:配置对象acl后复制对象
 * @Author wangkexin
 * @Date 2019.09.25
 */

public class SetObjectAcl19528 extends S3TestBase {
    private boolean runSuccess = false;
    private AmazonS3 s3Client = null;
    private String bucketName = "bucket19528";
    private String srcKeyName = "srcObj19528";
    private String dstKeyNamea = "trgObj19528a";
    private String dstKeyNameb = "trgObj19528b";
    private String dstKeyNamec = "trgObj19528c";
    private int fileSize = 6 * 1024 * 1024;
    private File localPath = null;
    private String filePath = null;
    private String ownerId;

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator
                + TestTools.getClassName() );
        filePath = localPath + File.separator + "localFile_" + fileSize
                + ".txt";
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( filePath, fileSize );

        s3Client = CommLib.buildS3Client();
        ownerId = s3Client.getS3AccountOwner().getId();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( bucketName );
        s3Client.putObject( bucketName, srcKeyName, new File( filePath ) );
    }

    @Test
    private void testSetObjectAcl() throws Exception {
        // set obejct acl
        Grant expGrant = new Grant( GroupGrantee.AuthenticatedUsers,
                Permission.ReadAcp );
        PrivilegeUtils.setObjectAclByHeader( s3AccessKeyId, bucketName,
                srcKeyName, expGrant );
        PrivilegeUtils.checkSetObjectAclResult( s3Client, bucketName,
                srcKeyName, expGrant );

        // test a:setMetadataDirective is null
        copyObjectAndCheckObjectAcl( dstKeyNamea, null );
        // test b:setMetadataDirective is COPY
        copyObjectAndCheckObjectAcl( dstKeyNameb, "COPY" );
        // test c:setMetadataDirective is REPLACE
        copyObjectAndCheckObjectAcl( dstKeyNamec, "REPLACE" );

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

    private void copyObjectAndCheckObjectAcl( String dstKeyName,
            String metadataDirective ) throws Exception {
        // put destinationKey
        s3Client.putObject( bucketName, dstKeyName,
                "test" + dstKeyName + "19528" );

        // copy object
        CopyObjectRequest request = new CopyObjectRequest( bucketName,
                srcKeyName, bucketName, dstKeyName );
        request.setMetadataDirective( metadataDirective );
        s3Client.copyObject( request );

        // check copy result
        String downfileMd5 = ObjectUtils.getMd5OfObject( s3Client, localPath,
                bucketName, srcKeyName );
        Assert.assertEquals( downfileMd5, TestTools.getMD5( filePath ) );

        downfileMd5 = ObjectUtils.getMd5OfObject( s3Client, localPath,
                bucketName, dstKeyName );
        Assert.assertEquals( downfileMd5, TestTools.getMD5( filePath ) );

        // check object acl
        Grant defaultGrant = new Grant( new CanonicalGrantee( ownerId ),
                Permission.FullControl );
        PrivilegeUtils.checkSetObjectAclResult( s3Client, bucketName,
                dstKeyName, defaultGrant );
    }
}
