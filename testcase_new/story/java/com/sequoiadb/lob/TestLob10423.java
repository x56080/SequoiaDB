package com.sequoiadb.lob;

import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Random;
import java.util.Vector;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @FileName:seqDB-10423:并发创建lob（相同lob和不同lob） 
 * 向cl中并发插入大量lob，并发量超过50
 * @Author linsuqiang
 * @Date 2016-12-12
 * @Version 1.00
 */
public class TestLob10423 extends SdbTestBase {
    private String clName = "cl_10423";
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private SimpleDateFormat sdf = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss.S");
    
    public class Md5Data{
        public ObjectId oid = null;
        public String md5 = null;
        public Md5Data(){}
        public Md5Data(ObjectId oid, String md5){
            this.oid = oid;
            this.md5 = md5;
        }
    }
    
    @BeforeClass
    public void setUp(){
        try{
            sdb = new Sequoiadb(SdbTestBase.coordUrl, "", "");
        }catch(BaseException e){            
            Assert.fail(e.getMessage());
        }
        createCL();
    }
    
    @AfterClass
    public void tearDown(){
        try{
            if(cs.isCollectionExist(clName)){
                cs.dropCollection(clName);
            }
        }catch(BaseException e){    
            Assert.fail(e.getMessage());
        }finally{
            sdb.disconnect();
        }
    }
    
    @Test
    public void test(){
        Vector<Md5Data> prevMd5Data = new Vector<Md5Data>();
        PutLobsThread putLobsThread = new PutLobsThread(prevMd5Data);
        putLobsThread.start(60);
        if(!putLobsThread.isSuccess()){
            Assert.fail(putLobsThread.getErrorMsg());
        }
        checkMd5(prevMd5Data);
    }
    
    private class PutLobsThread extends SdbThreadBase {
        private Vector<Md5Data> prevMd5Data = null;
        public PutLobsThread(Vector<Md5Data> prevMd5Data){
            this.prevMd5Data = prevMd5Data;
        }
        @Override
        public void exec() throws BaseException{
            Sequoiadb db = null;
            DBCollection cl = null;
            try{
                db = new Sequoiadb(SdbTestBase.coordUrl, "", "");
                cl = db.getCollectionSpace(csName).getCollection(clName);
                // do put lobs
                String []lobStrs = buildLobStrs();
                putLobs(cl, lobStrs, prevMd5Data);
            }catch(BaseException e){
                throw e;
            }finally{
                db.disconnect();
            }
        }
    }

    private void createCL(){
        try{
            if(!sdb.isCollectionSpaceExist(SdbTestBase.csName)){
                sdb.createCollectionSpace(SdbTestBase.csName);    
            }
        }catch(BaseException e){
            //-33 CS exist,ignore exceptions
            Assert.assertEquals(-33,e.getErrorCode(),e.getMessage());
        }
        try{
            cs = sdb.getCollectionSpace(SdbTestBase.csName);
            BSONObject options = new BasicBSONObject();
            options = (BSONObject)JSON.parse("{ShardingKey:{a:1,b:-1},ShardingType:'hash',Partition:4096}");
            cs.createCollection(clName, options);    
        }catch(BaseException e){
            Assert.assertTrue(false,"create cl fail "+e.getErrorType()+":"+e.getMessage());
        }
    }
    
    private String[] buildLobStrs(){
        // build lobs(including some same and some different every time)
        // same lobs
        String []sameLob = { "aabbccdddeeedeekrr" , 
                "jjjkkikjijkjikjijjiikjghghghg", 
                "oioioioioioioioioioi" };
        // different lobs
        String []diffLob = new String[5];
        Random random = new Random();
        for(int i = 0; i < diffLob.length; i++){
            int lobsize = random.nextInt(1048);
            diffLob[i] = LobUtils.getRandomString(lobsize);
        }
        // merged lobs
        String []lobStrs = new String[ sameLob.length + diffLob.length ];
        System.arraycopy(sameLob, 0, lobStrs, 0, sameLob.length);
        System.arraycopy(diffLob, 0, lobStrs, sameLob.length, diffLob.length);
        
        return lobStrs;
    }
            
    private void putLobs(DBCollection cl, String lobStrs[], Vector<Md5Data> prevMd5Data){
        for (int i = 0; i < lobStrs.length; i++){
            DBLob lob = null;
            try{
                lob = cl.createLob();
                lob.write(lobStrs[i].getBytes());
                ObjectId currOid = lob.getID();
                String currMd5 = LobUtils.getMd5(lobStrs[i]);   
                prevMd5Data.add(new Md5Data(currOid, currMd5));
            }catch(BaseException e){
                throw e;
            }finally{
                if(lob != null){
                    lob.close();
                }
            }
        }
    }
    
    private void checkMd5(Vector<Md5Data> prevMd5Data){
        DBCollection cl = sdb.getCollectionSpace(csName).getCollection(clName);
        for(int i = 0; i < prevMd5Data.size(); i++){
            try{
                DBLob rLob = cl.openLob(prevMd5Data.get(i).oid);
                byte[] bytebuff = new byte[(int)rLob.getSize()];
                rLob.read(bytebuff);
                String curMd5 = LobUtils.getMd5(bytebuff);
                Assert.assertEquals(curMd5, prevMd5Data.get(i).md5,"the lobs md5 different");
            }catch(BaseException e){
                Assert.fail(e.getMessage());
            }
        }
    }
}
