package com.mongodb.java.concurrent;

import java.io.File;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.List;
import java.util.Random;
import java.util.Set;

import org.testng.Assert;
import org.testng.ITestContext;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.mongodb.BasicDBObject;
import com.mongodb.DB;
import com.mongodb.DBObject;
import com.mongodb.QueryBuilder;
import com.mongodb.gridfs.GridFS;
import com.mongodb.gridfs.GridFSDBFile;
import com.mongodb.gridfs.GridFSInputFile;
import com.mongodb.utils.MongodbTestBase;
import com.mongodb.utils.TestTools;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @Description: seqDB-22081:并发增删改查gridfs文件
 * @author fanyu
 * @Date 2020/4/8
 * @version 1.00
 */
public class GridFS22081 extends MongodbTestBase {
    private DB db;
    private File localPath;
    private String filNameBase = "fs22081";
    private String bucketName;
    private int fileNum = 10;
    private byte[] bytes = new byte[ 1024 ];
    private List< String > fileIdList = new ArrayList<>();

    @BeforeClass
    public void setUp() throws UnknownHostException {
        localPath = new File( File.separator + "tmp" + File.separator
                + File.separator + TestTools.getClassName() );
        TestTools.LocalFile.removeFile( localPath );
        TestTools.LocalFile.createDir( localPath.toString() );
        db = MongodbTestBase.getDB( client );
        bucketName = javaDBNameWithVersion + "_bucket22081";

        Set< String > clNames = db.getCollectionNames();
        for ( String clName : clNames ) {
            if ( clName.startsWith( bucketName ) ) {
                db.getCollection( clName ).drop();
            }
        }
        new Random().nextBytes( bytes );
        // 创建文件用于删除
        GridFS gridFS = new GridFS( db, bucketName );
        for ( int i = 0; i < fileNum; i++ ) {
            GridFSInputFile gfile = gridFS.createFile( bytes );
            gfile.setFilename( filNameBase + i );
            gfile.put( "a", i );
            gfile.save();
            fileIdList.add( gfile.getId().toString() );
        }
        // 创建文件用于更新
        for ( int i = 0; i < fileNum; i++ ) {
            GridFSInputFile gfile = gridFS.createFile( bytes );
            gfile.setFilename( filNameBase + i );
            gfile.put( "b", i );
            gfile.save();
            fileIdList.add( gfile.getId().toString() );
        }
    }

    @Test
    public void test1() throws Exception {
        ThreadExecutor threadExec = new ThreadExecutor();
        int threadNum = 10;
        // 创建和查询有交集， 删除和更新无交集， 查询与删除有交集
        for ( int i = 0; i < threadNum; i++ ) {
            // 创建
            threadExec.addWorker(
                    new Create( filNameBase + ( fileNum + i ), fileNum + i ) );
            // 删除
            threadExec.addWorker(
                    new Delete( QueryBuilder.start( "a" ).is( i ).get() ) );
            // 更新
            threadExec.addWorker( new Update(
                    QueryBuilder.start( "b" ).is( i ).get(),
                    new BasicDBObject( "c", i ).append( "b", fileNum + i ) ) );
            // 查询
            threadExec.addWorker( new Query(
                    QueryBuilder.start( "a" ).exists( true ).get() ) );
        }
        // 删除与更新有交集
        threadExec.addWorker(
                new Update( QueryBuilder.start( "a" ).exists( true ).get(),
                        new BasicDBObject( "a", 0 ).append( "c", 1 ) ) );
        threadExec.run();

    }

    @AfterClass
    public void tearDown( ITestContext context ) {
        String[] clNames = { bucketName + ".files", bucketName + ".chunks" };
        dropCLByTestResult( context, this.toString(), db, clNames );
        TestTools.LocalFile.removeFile( localPath );
    }

    private class Create {
        private String fileName;
        private int a;

        public Create( String fileName, int a ) {
            this.fileName = fileName;
            this.a = a;
        }

        @ExecuteOrder(step = 1)
        private void create() {
            GridFS gridFS = new GridFS( db, bucketName );
            GridFSInputFile gfile = gridFS.createFile( bytes );
            gfile.setFilename( fileName );
            gfile.put( "a", a );
            gfile.save();
        }
    }

    private class Delete {
        private DBObject query;

        public Delete( DBObject query ) {
            this.query = query;
        }

        @ExecuteOrder(step = 1)
        private void delete() {
            GridFS gridFS = new GridFS( db, bucketName );
            gridFS.remove( query );
        }
    }

    private class Update {
        private DBObject query;

        public Update( DBObject query, DBObject update ) {
            this.query = query;
        }

        @ExecuteOrder(step = 1)
        private void update() {
            GridFS gridFS = new GridFS( db, bucketName );
            List< GridFSDBFile > files = gridFS.find( query );
            for ( GridFSDBFile file : files ) {
                file.put( "a", 1 );
                file.save();
            }
        }
    }

    private class Query {
        private DBObject query;

        public Query( DBObject query ) {
            this.query = query;
        }

        @ExecuteOrder(step = 1)
        private void query() {
            GridFS gridFS = new GridFS( db, bucketName );
            List< GridFSDBFile > files = gridFS.find( query );
            Assert.assertTrue( files.size() >= 0, files.toString() );
        }
    }
}
