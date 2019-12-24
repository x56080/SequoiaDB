package com.sequoias3.privilege.concurrent;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AccessControlList;
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
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import com.sequoias3.testcommon.s3utils.PrivilegeUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 * @Description seqDB-19479: 并发配置对象acl和更新对象
 * @Author wangkexin
 * @Date 2019.09.25
 */
public class SetObjectAclAndPutObject19479 extends S3TestBase {
    private String bucketName = "bucket19479";
    private String keyName = "key19479";
    private AmazonS3 s3Client = null;
    private long oldFileSize = 100 * 1024;
    private long newFileSize = 200 * 1024;
    private File localPath = null;
    private File oldFile = null;
    private File newFile = null;
    private String oldFilePath = null;
    private String newFilePath = null;
    private String ownerId;
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator + TestTools
                .getClassName() );
        oldFilePath = localPath + File.separator + "localFile_" + oldFileSize
                + ".txt";
        newFilePath = localPath + File.separator + "localFile_" + newFileSize
                + ".txt";

        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        TestTools.LocalFile.createFile( oldFilePath, oldFileSize );
        TestTools.LocalFile.createFile( newFilePath, newFileSize );
        oldFile = new File( oldFilePath );
        newFile = new File( newFilePath );

        s3Client = CommLib.buildS3Client();
        CommLib.clearBucket( s3Client, bucketName );
        s3Client.createBucket( new CreateBucketRequest( bucketName ) );
        ownerId = s3Client.getS3AccountOwner().getId();
        s3Client.putObject( bucketName, keyName, oldFile );
    }

    @Test
    private void test() throws Exception {
        Grant[] expGrant = { new Grant( new CanonicalGrantee( ownerId ),
                Permission.ReadAcp ),
                new Grant( GroupGrantee.AllUsers, Permission.Read ) };
        Grant[] defaultGrant = { new Grant( new CanonicalGrantee( ownerId ),
                Permission.FullControl ) };

        ThreadExecutor threadExec = new ThreadExecutor();
        threadExec.addWorker( new ThreadSetObjectAcl( expGrant ) );
        threadExec.addWorker( new ThreadPutObject( newFile ) );
        threadExec.run();

        // check object acl
        List<Grant> expGrantsList = null;
        AccessControlList result = s3Client.getObjectAcl( bucketName, keyName );
        List<Grant> actGrantsList = result.getGrantsAsList();
        if ( actGrantsList.size() == defaultGrant.length ) {
            expGrantsList = new ArrayList<>( Arrays.asList( defaultGrant ) );
        } else if ( actGrantsList.size() == expGrant.length ) {
            expGrantsList = new ArrayList<>( Arrays.asList( expGrant ) );
        } else {
            Assert.fail( "act object acl size is wrong : " + actGrantsList
                    .toString() );
        }
        checkGrantList( actGrantsList, expGrantsList );

        // check object
        String expMd5 = TestTools.getMD5( newFilePath );
        String downloadMd5 = ObjectUtils
                .getMd5OfObject( s3Client, localPath, bucketName, keyName );
        Assert.assertEquals( downloadMd5, expMd5 );
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

    private void checkGrantList( List<Grant> actGrantsList,
            List<Grant> expGrantsList ) {
        boolean isEqual = false;
        if ( actGrantsList.size() == expGrantsList.size() && actGrantsList
                .containsAll( expGrantsList ) && expGrantsList
                .containsAll( actGrantsList ) ) {
            isEqual = true;
        }
        if ( !isEqual ) {
            Assert.fail( "object acl is wrong! exp grants = " + expGrantsList
                    .toString() + ", act grants = " + actGrantsList
                    .toString() );
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
            } finally {
                if ( s3 != null ) {
                    s3.shutdown();
                }
            }
        }
    }

    private class ThreadPutObject {
        private AmazonS3 s3 = CommLib.buildS3Client();
        private File file;

        public ThreadPutObject( File file ) {
            this.file = file;
        }

        @ExecuteOrder(step = 1)
        private void putObject() {
            try {
                s3.putObject( bucketName, keyName, file );
            } finally {
                if ( s3 != null ) {
                    s3.shutdown();
                }
            }
        }
    }
}
