package com.sequoias3.partupload.concurrent;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.ListPartsRequest;
import com.amazonaws.services.s3.model.PartETag;
import com.amazonaws.services.s3.model.PartListing;
import com.amazonaws.services.s3.model.PartSummary;
import com.amazonaws.services.s3.model.UploadPartRequest;
import com.amazonaws.services.s3.model.UploadPartResult;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;
import com.sequoias3.testcommon.CommLib;
import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestTools;
import com.sequoias3.testcommon.s3utils.ObjectUtils;
import com.sequoias3.testcommon.s3utils.PartUploadUtils;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.io.RandomAccessFile;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Set;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * @Description seqDB-18681:关闭检测开关，上传第一个partNumber被覆盖写
 * @Author huangxiaoni
 * @Date 2019.07.26
 */

public class UploadPart18681 extends S3TestBase {
    private AtomicInteger actSuccessTests = new AtomicInteger( 0 );
    private File localPath;
    private String filePath1;
    private String filePath2;
    private String filePath3;
    private File file1;
    private File file2;
    private int fileSize = 20 * 1024 * 1024;
    private int fileId = 0;

    private AmazonS3 s3Client;
    private String keyBase = "/aa/bb/obj18681";
    private List< String > keys = new ArrayList<>();
    private String uploadId;
    private Map< File, PartETag > firstPartETags = new HashMap< File, PartETag >();

    @DataProvider(name = "firstPartSize")
    private Object[][] generateFirstPartSize() {
        // parameter : firstPartNumber1, firstPartNumber2, firstPartSize1,
        // firstPartSize2, key
        return new Object[][] {
                // firstPartNumber, firstPartSize1, firstPartSize2, key
                new Object[] { 1, 5 * 1024 * 1024, 5 * 1024 * 1024,
                        keyBase + "_1" },
                new Object[] { 1, 5 * 1024 * 1024, 6 * 1024 * 1024,
                        keyBase + "_2" },
                new Object[] { 2, 5 * 1024 * 1024, 5 * 1024 * 1024,
                        keyBase + "_3" },
                new Object[] { 2, 5 * 1024 * 1024, 6 * 1024 * 1024,
                        keyBase + "_4" } };
    }

    @BeforeClass
    private void setUp() throws IOException {
        this.initFile();
        s3Client = CommLib.buildS3Client();
    }

    @Test(dataProvider = "firstPartSize")
    private void test( int firstPartNumber, int firstPartSize1,
            int firstPartSize2, String key ) throws Exception {
        filePath3 = this.createFile( 0 );
        keys.add( key );
        List< PartETag > partETags = new ArrayList<>();

        // init part upload
        uploadId = PartUploadUtils.initPartUpload( s3Client,
                S3TestBase.bucketName, key );

        // upload first part
        ThreadExecutor threadExec = new ThreadExecutor();
        threadExec.addWorker( new ThreadUploadFirstPart( file1, key,
                firstPartNumber, firstPartSize1 ) );
        threadExec.addWorker( new ThreadUploadFirstPart( file2, key,
                firstPartNumber, firstPartSize2 ) );
        threadExec.run();
        Assert.assertEquals( firstPartETags.size(), 2 );

        // get first part info, make sure the successful part
        // get the firstPartETagStr
        ListPartsRequest request = new ListPartsRequest( bucketName, key,
                uploadId );
        PartListing parts = s3Client.listParts( request );
        PartSummary partSummary = parts.getParts().get( 0 );
        // get the firstPartETag
        File actFile = null;
        PartETag firstPartETag = null;
        Set< Entry< File, PartETag > > entrySet = firstPartETags.entrySet();
        Iterator< Map.Entry< File, PartETag > > it = entrySet.iterator();
        while ( it.hasNext() ) {
            Map.Entry< File, PartETag > entry = it.next();
            actFile = entry.getKey();
            firstPartETag = entry.getValue();
            if ( firstPartETag.getETag().equals( partSummary.getETag() ) ) {
                break;
            }
        }
        partETags.add( firstPartETag );

        // upload others part
        PartETag otherPartETag = this.uploadOthersPart( key,
                partSummary.getSize(), firstPartNumber );
        partETags.add( otherPartETag );

        // upload
        PartUploadUtils.completeMultipartUpload( s3Client, bucketName, key,
                uploadId, partETags );

        // check results
        fileId++;
        File downloadPath = new File( localPath + File.separator
                + "downloadFile_" + fileSize + "_" + fileId );
        String downfileMd5 = ObjectUtils.getMd5OfObject( s3Client, downloadPath,
                bucketName, key );
        String expFilePath = filePath1;
        if ( actFile == file2 ) {
            expFilePath = filePath3;
            int remainPartSize = fileSize - firstPartSize2;
            this.readFile( filePath2, 0, firstPartSize2, filePath3 );
            this.readFile( filePath1, firstPartSize2, remainPartSize,
                    filePath3 );
        }
        Assert.assertEquals( downfileMd5, TestTools.getMD5( expFilePath ),
                "expFilePath = " + expFilePath );

        // clear
        actSuccessTests.getAndIncrement();
        TestTools.LocalFile.removeFile( filePath3 );
        TestTools.LocalFile.removeFile( downloadPath );
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( actSuccessTests.get() == generateFirstPartSize().length ) {
                for ( String key : keys ) {
                    s3Client.deleteObject( S3TestBase.bucketName, key );
                }
                TestTools.LocalFile.removeFile( localPath );
            }
        } finally {
            s3Client.shutdown();
        }
    }

    private PartETag uploadOthersPart( String key, long fileOffset,
            int partNumberBase ) {
        long remainPartSize = fileSize - fileOffset;
        UploadPartRequest partRequest = new UploadPartRequest()
                .withFile( file1 ).withFileOffset( fileOffset )
                .withPartNumber( partNumberBase + 1 )
                .withPartSize( remainPartSize ).withBucketName( bucketName )
                .withKey( key ).withUploadId( uploadId );
        UploadPartResult partResult = s3Client.uploadPart( partRequest );
        return partResult.getPartETag();
    }

    private void initFile() throws IOException {
        localPath = new File( S3TestBase.workDir + File.separator
                + TestTools.getClassName() );
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );

        filePath1 = this.createFile( fileSize );
        filePath2 = this.createFile( fileSize );
        file1 = new File( filePath1 );
        file2 = new File( filePath2 );
    }

    private String createFile( int fileSize ) throws IOException {
        fileId++;
        String filePath = localPath + File.separator + "localFile_" + fileSize
                + "_" + fileId + ".txt";
        TestTools.LocalFile.createFile( filePath, fileSize );
        return filePath;
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

    private class ThreadUploadFirstPart {
        private File file;
        private String key;
        private int partNumer;
        private long partSize;

        public ThreadUploadFirstPart( File file, String key, int partNumer,
                long partSize ) {
            this.file = file;
            this.key = key;
            this.partNumer = partNumer;
            this.partSize = partSize;
        }

        @ExecuteOrder(step = 1)
        private void uploadPart() {
            AmazonS3 s3 = null;
            try {
                s3 = CommLib.buildS3Client();
                UploadPartRequest partRequest = new UploadPartRequest()
                        .withFile( file ).withFileOffset( 0 )
                        .withPartNumber( partNumer ).withPartSize( partSize )
                        .withBucketName( bucketName ).withKey( key )
                        .withUploadId( uploadId );
                UploadPartResult partResult = s3.uploadPart( partRequest );
                firstPartETags.put( file, partResult.getPartETag() );
            } finally {
                if ( s3 != null ) {
                    s3.shutdown();
                }
            }
        }
    }
}