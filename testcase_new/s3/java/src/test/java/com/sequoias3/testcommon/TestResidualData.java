package com.sequoias3.testcommon;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;

public class TestResidualData extends S3TestBase {
    private Sequoiadb db;
    private int errorCount = 0;
    private String bucketId;
    private String enabledBucketId;
    private String residualdataFilePath;
    private String residualDirDataFilePath;

    @BeforeClass
    private void setUp() throws InterruptedException, IOException {
        // 打印残留信息前等待一分钟（s3后台清理频率为1次/分钟）
        Thread.sleep( 60 * 1024 );
        db = new Sequoiadb( S3TestBase.coordUrl, "", "" );
        residualdataFilePath =
                S3TestBase.workDir + File.separator + "residualdata.log";
        residualDirDataFilePath =
                S3TestBase.workDir + File.separator + "residualDirData.log";
        TestTools.LocalFile.createFile( residualdataFilePath );
        TestTools.LocalFile.createFile( residualDirDataFilePath );
    }

    @Test
    private void printResidualData() throws Exception {
        List< String > csNames;
        List< String > s3CSNames = new ArrayList<>();
        List< String > s3DataCSNames = new ArrayList<>();

        csNames = db.getCollectionSpaceNames();
        //get s3 tables
        for ( String csName : csNames ) {
            if ( csName.startsWith( "S3_" ) ) {
                if ( csName.contains( "Meta" ) ) {
                    s3CSNames.add( csName );
                } else {
                    s3DataCSNames.add( csName );
                }
            }
        }

        // residual meta data and lod  write to local file
        for ( String csName : s3CSNames ) {
            CollectionSpace cs = db.getCollectionSpace( csName );
            List< DBCollection > clList = new ArrayList< DBCollection >();
            List< String > clNameList = cs.getCollectionNames();
            for ( String csclName : clNameList ) {
                String clname = csclName.substring( cs.getName().length() + 1 );
                // S3_SYS_Meta
                // .S3_IDGenerator为ID生成表，属于s3内部表，不需要打印和校验，S3_SYS_Meta
                // .S3_ObjectDir目录表单独校验打印
                if ( !clname.equals( "S3_IDGenerator" ) && !clname
                        .equals( "S3_ObjectDir" ) ) {
                    clList.add( cs.getCollection( clname ) );
                }
            }
            residualMetaDataWriteToLocalFile( cs, clList );
            residualObjectDirDataWriteToLocalFile( cs );
        }

        // residual meta dir  write to local file
        for ( String csName : s3DataCSNames ) {
            CollectionSpace cs = db.getCollectionSpace( csName );
            List< DBCollection > clList = new ArrayList< DBCollection >();
            List< String > clNameList = cs.getCollectionNames();
            for ( String csclName : clNameList ) {
                String clname = csclName.substring( cs.getName().length() + 1 );
                clList.add( cs.getCollection( clname ) );
            }
            residualDataWriteToLocalFile( cs, clList );
        }
        //scp to s3 host
        residualFileScpToS3Host();
    }

    private void residualMetaDataWriteToLocalFile( CollectionSpace cs,
            List< DBCollection > clList ) throws IOException {
        DBCursor cursor = null;
        SimpleDateFormat sdf = new SimpleDateFormat( "yyyy" );
        Date date = new Date();
        String s3SysDataRegionSpaceName =
                "S3_SYS_Data_" + sdf.format( date );
        FileOutputStream fileOutputStream = new FileOutputStream(
                new File( residualdataFilePath ), true );
        try {
            for ( DBCollection cl : clList ) {
                cursor = cl.query();
                String head =
                        "===============begin print " + cs.getName() + "." + cl
                                .getName() + " data============\n";
                fileOutputStream.write( head.getBytes() );
                while ( cursor.hasNext() ) {
                    if ( cursor.getNext().containsField( "Name" ) ) {
                        if ( !cursor.getCurrent().get( "Name" )
                                .equals( S3TestBase.s3UserName ) && !cursor
                                .getCurrent().get( "Name" )
                                .equals( S3TestBase.bucketName ) && !cursor
                                .getCurrent().get( "Name" )
                                .equals( S3TestBase.enableVerBucketName )
                                && !( String
                                .valueOf( cursor.getCurrent().get( "Name"
                                ) ).startsWith( s3SysDataRegionSpaceName ) ) ) {
                            fileOutputStream
                                    .write( ( cursor.getCurrent().toString()
                                            + "\n" ).getBytes() );
                            errorCount++;
                        } else if ( cursor.getCurrent().get( "Name" )
                                .equals( S3TestBase.bucketName ) ) {
                            bucketId = cursor.getCurrent().get( "ID" )
                                    .toString();
                        } else if ( cursor.getCurrent().get( "Name" )
                                .equals( S3TestBase.enableVerBucketName ) ) {
                            enabledBucketId = cursor.getCurrent().get( "ID" )
                                    .toString();
                        }
                    } else {
                        fileOutputStream.write( ( cursor.getCurrent().toString()
                                + "\n" ).getBytes() );
                        errorCount++;
                    }
                }
                String tail =
                        "===============end print " + cs.getName() + "." + cl
                                .getName() + " data==============\n";
                fileOutputStream.write( tail.getBytes() );
            }
        } finally {
            if ( cursor != null ) {
                cursor.close();
            }
            if ( fileOutputStream != null ) {
                fileOutputStream.close();
            }
        }
    }

    private void residualDataWriteToLocalFile( CollectionSpace cs,
            List< DBCollection > clList ) throws IOException {
        DBCursor cursor = null;
        FileOutputStream fileOutputStream = new FileOutputStream(
                new File( residualdataFilePath ), true );
        try {
            for ( DBCollection cl : clList ) {
                String head =
                        "\n===============begin print " + cs.getName() + "."
                                + cl.getName() + " data============\n";
                fileOutputStream.write( head.getBytes() );
                cursor = cl.listLobs();
                if ( cursor.hasNext() ) {
                    while ( cursor.hasNext() ) {
                        fileOutputStream
                                .write( ( cursor.getNext().toString() + "\n" )
                                        .getBytes() );
                        errorCount++;
                    }
                }
                String tail =
                        "===============end print " + cs.getName() + "." + cl
                                .getName() + " data==============\n";
                fileOutputStream.write( tail.getBytes() );
            }
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(),
                    SDBError.SDB_DMS_NOTEXIST.getErrorCode(),
                    "getCollection ObjectDataList failed" );
        } finally {
            if ( cursor != null ) {
                cursor.close();
            }
            if ( fileOutputStream != null ) {
                fileOutputStream.close();
            }
        }
    }

    private void residualObjectDirDataWriteToLocalFile( CollectionSpace cs )
            throws IOException {
        DBCollection cl = cs.getCollection( "S3_ObjectDir" );
        DBCursor cursor = null;
        FileOutputStream fileOutputStream = new FileOutputStream(
                new File( residualDirDataFilePath ), true );
        try {
            String head =
                    "===============begin print " + cs.getName() + "." + cl
                            .getName() + " data============\n";
            fileOutputStream.write( head.getBytes() );
            cursor = cl.query();
            if ( cursor.hasNext() ) {
                while ( cursor.hasNext() ) {
                    if ( !cursor.getNext().get( "BucketId" ).equals( bucketId )
                            && !cursor.getCurrent().get( "BucketId" )
                            .equals( enabledBucketId ) ) {
                        fileOutputStream.write( ( cursor.getCurrent().toString()
                                + "\n" ).getBytes() );
                        errorCount++;
                    }
                }
            }
            String tail = "===============end print " + cs.getName() + "." + cl
                    .getName() + " data==============\n";
            fileOutputStream.write( tail.getBytes() );
        } finally {
            if ( cursor != null ) {
                cursor.close();
            }
            if ( fileOutputStream != null ) {
                fileOutputStream.close();
            }
        }
    }

    private void residualFileScpToS3Host() throws Exception {
        if ( errorCount != 0 ) {
            String remotPath = S3TestBase.installPath + "/tools/sequoias3/log";
            copyLocalFileToRemote( residualdataFilePath, remotPath );
            copyLocalFileToRemote( residualDirDataFilePath, remotPath );
            throw new Exception( "There is data residue problem" );
        }
    }

    private void copyLocalFileToRemote( String localFilePath,
            String remotePath ) throws Exception {
        Ssh ssh = null;
        try {
            ssh = new Ssh( s3HostName, remoteuser, remotepasswd );
            ssh.scpTo( localFilePath, remotePath );
            if ( ssh.getExitStatus() != 0 ) {
                throw new Exception(
                        "exec ssh.scpTo(" + localFilePath + ", " + remotePath
                                + "); failed, stout= " + ssh.getStdout() );
            }
        } catch ( Exception e ) {
            e.printStackTrace();
            Assert.fail( "write " + remotePath + " info failed" );
        } finally {
            if ( ssh != null ) {
                ssh.disconnect();
            }
        }
    }

    @AfterClass
    private void tearDown() throws Exception {
        //delete local file
        TestTools.LocalFile.removeFile( residualdataFilePath );
        TestTools.LocalFile.removeFile( residualDirDataFilePath );
        if ( db != null ) {
            db.close();
        }
    }
}
