package com.sequoias3.partupload;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.PartETag;
import com.amazonaws.services.s3.model.UploadPartRequest;
import com.amazonaws.services.s3.model.UploadPartResult;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import com.sequoias3.testcommon.s3utils.PartUploadUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.io.RandomAccessFile;
import java.util.ArrayList;
import java.util.List;

/**
 * @Description seqDB-18680:上传相同分段，长度相同内容不同
 * @Author huangxiaoni
 * @Date 2019.07.26
 */

public class UploadPart18680 extends S3TestBase {
    List<PartETag> partETags = new ArrayList<>();
    private boolean runSuccess = false;
    private AmazonS3 s3Client;
    private File localPath;
    private String filePath1;
    private String filePath2;
    private String filePath3;
    private File file1;
    private File file2;
    private int fileSize = 11 * 1024 * 1024;
    private int firstPartSize = 5 * 1024 * 1024;
    private int remainPartSize = fileSize - firstPartSize;
    private String key = "/aa/bb/obj18680";

    @BeforeClass
    private void setUp() throws IOException {
        this.initFile();
        s3Client = CommLib.buildS3Client();
    }

    @Test
    private void test() throws Exception {
        String uploadId = PartUploadUtils
                .initPartUpload( s3Client, S3TestBase.bucketName, key );
        this.partUpload( uploadId );
        PartUploadUtils
                .completeMultipartUpload( s3Client, bucketName, key, uploadId,
                        partETags );

        // check results
        String downfileMd5 = ObjectUtils
                .getMd5OfObject( s3Client, localPath, bucketName, key );
        Assert.assertEquals( downfileMd5, TestTools.getMD5( filePath3 ) );

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess ) {
                s3Client.deleteObject( S3TestBase.bucketName, key );
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            s3Client.shutdown();
        }
    }

    private void partUpload( String uploadId ) {
        File file = file1;
        long fileOffset = 0;
        int partNum = 1;
        long partSize = firstPartSize;
        for ( int i = 0; i < 3; i++ ) {
            if ( i >= 1 ) {
                fileOffset = firstPartSize;
                partNum = 2;
                partSize = remainPartSize;
                if ( i == 2 ) {
                    file = file2;
                }
            }
            UploadPartRequest partRequest = new UploadPartRequest()
                    .withFile( file ).withFileOffset( fileOffset )
                    .withPartNumber( partNum ).withPartSize( partSize )
                    .withBucketName( bucketName ).withKey( key )
                    .withUploadId( uploadId );
            UploadPartResult partResult = s3Client.uploadPart( partRequest );
            if ( i == 0 || i == 2 ) {
                partETags.add( partResult.getPartETag() );
            }
        }
    }

    private void initFile() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator + TestTools
                .getClassName() );
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );

        String filePathBase =
                localPath + File.separator + "localFile_" + fileSize;
        filePath1 = filePathBase + "_1.txt";
        filePath2 = filePathBase + "_2.txt";
        filePath3 = filePathBase + "_3.txt";
        TestTools.LocalFile.createFile( filePath1, fileSize );
        TestTools.LocalFile.createFile( filePath2, fileSize );
        file1 = new File( filePath1 );
        file2 = new File( filePath2 );

        // expect file content
        TestTools.LocalFile.createFile( filePath3, 0 );
        this.readFile( filePath1, 0, firstPartSize, filePath3 );
        this.readFile( filePath2, firstPartSize, remainPartSize, filePath3 );
    }

    private void readFile( String filePath, int off, int len,
            String downloadPath ) throws FileNotFoundException, IOException {
        RandomAccessFile raf = null;
        OutputStream fos = null;
        try {
            raf = new RandomAccessFile( filePath, "rw" );
            fos = new FileOutputStream( downloadPath, true );
            int size = off;
            raf.seek( size );
            int readSize = 0;
            byte[] buf = new byte[ off + len ];
            readSize = raf.read( buf, off, len );
            fos.write( buf, off, readSize );
        } finally {
            if ( raf != null )
                raf.close();
            if ( fos != null )
                fos.close();
        }
    }
}