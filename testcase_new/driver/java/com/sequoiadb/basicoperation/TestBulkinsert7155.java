package com.sequoiadb.basicoperation;

import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.regex.Pattern;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BSONDecimal;
import org.bson.types.BasicBSONList;
//import org.bson.types.Binary;
import org.bson.types.ObjectId;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
* FileName: TestBulkinsert7155.java
* test interface:bulkInsert (List< BSONObject > insertor, int flag) 
* and test flag values:FLG_INSERT_CONTONDUP,0
* @author wuyan
    * @Date    2016.9.12
* @version 1.00
*/
public class TestBulkinsert7155 extends SdbTestBase{	
	private String clName = "cl_7155";
	private static Sequoiadb sdb = null;
	private CollectionSpace cs = null;
	private DBCollection cl ;
	
	@BeforeClass
	public void setUp( ){
		System.out.println(this.getClass().getName()+" begin at "
				+new SimpleDateFormat("yyyy-MM-dd HH:mm:ss:S").format(new Date()));
		try{
			sdb = new Sequoiadb(SdbTestBase.coordUrl, "", "");
			sdb.setSessionAttr(new BasicBSONObject("PreferedInstance", "M")); 
		}catch(BaseException e){			
			Assert.assertTrue(false,"connect %s failed,"+SdbTestBase.coordUrl+e.getMessage());
		}		
		createCL();
	}
	
	private void createCL(){
		try{
			if (!sdb.isCollectionSpaceExist(SdbTestBase.csName)){
				sdb.createCollectionSpace(SdbTestBase.csName);	
			}
		}catch(BaseException e){
			//-33 CS exist,ignore exceptions
			Assert.assertEquals(-33,e.getErrorCode(),e.getMessage());
		}
		String test = "{ReplSize:0,Compressed:true}";
		BSONObject options =(BSONObject) JSON.parse(test);
		try
		{
			cs = sdb.getCollectionSpace(SdbTestBase.csName);			
			cl = cs.createCollection(clName,options);
		}catch(BaseException e){
			Assert.assertTrue(false,"create cl fail "+e.getErrorType()+":"+e.getMessage());
		}
	}
	
	/**
	*test bulkInsert (List< BSONObject > insertor, int flag)��set flag=FLG_INSERT_CONTONDUP 
	*/
	public void bulkInsert(){
		try{
			List<BSONObject>list = new ArrayList<BSONObject>();			
			long num = 2;			
			for ( long i = 0; i < num; i++){				
				BSONObject obj = new BasicBSONObject();
				ObjectId id = new ObjectId();
				obj.put("_id", id);
				//insert the decimal type data
				String str = "32345.067891234567890123456789" + i;
				BSONDecimal decimal = new BSONDecimal(str);			
				obj.put("decimal",decimal);
				obj.put("no", i);				
				obj.put("str", "test_" + String.valueOf(i));
				//the numberlong type data
				BSONObject numberlong = new BasicBSONObject();
				numberlong.put("$numberLong","-9223372036854775808");			
				obj.put("numlong",numberlong);
				//the obj type
				BSONObject subObj = new BasicBSONObject();
				subObj.put("a",1+i);
				obj.put("obj",subObj);
				//the array type
				BSONObject arr = new BasicBSONList();	
				arr.put("0", (int) (Math.random() * 100));
				arr.put("1","test");
				arr.put("2",2.34);
				obj.put("arr",arr);
				obj.put("boolf",false);
				//the data type 
				Date now = new Date();
				obj.put("date",now);
				//the regex type
				Pattern regex = Pattern.compile("^2001",Pattern.CASE_INSENSITIVE);
				obj.put("binary", regex);			
				list.add(obj);				
			}
			cl.bulkInsert(list, DBCollection.FLG_INSERT_CONTONDUP);			
			
			//check the bulkInsert result
			BSONObject tmp = new BasicBSONObject();
	        DBCursor tmpCursor = cl.query(tmp, null, null, null);
	        BasicBSONObject temp = null;
	        List<BSONObject>listActDatas = new ArrayList<BSONObject>();
	        while(tmpCursor.hasNext()){
	            temp = (BasicBSONObject)tmpCursor.getNext();
	            listActDatas.add(temp);
	        }	        
	        Assert.assertEquals(listActDatas.equals(list),true,"check datas are unequal\n"+"actDatas: "+listActDatas.toString());
		}catch(BaseException e){
			Assert.assertTrue(false,"bulkinsert fail "+e.getErrorCode()+e.getMessage());
		}		
	}
	
	/**
	*test bulkInsert (List< BSONObject > insertor, int flag)��set flag=0 
	*/
	public void bulkInsertDuplicateKey(){
		try{
			List<BSONObject>list = new ArrayList<BSONObject>();		
			for ( long i = 0; i < 2; i++){
				BSONObject obj = new BasicBSONObject();				
				obj.put("_id", 1);
				String str = "32345.067891234567890123456789";
				BSONDecimal decimal = new BSONDecimal(str);			
				obj.put("decimal",decimal);
				obj.put("no", 5);		
				list.add(obj);				
			}
			try{
				cl.bulkInsert(list, 0);				
				Assert.fail("bulkInsert will interrupt when Duplicate key exist");
			}catch(BaseException e){
				Assert.assertEquals(e.getErrorCode(),-38,e.getMessage());
			}			
			long count = cl.getCount();
			Assert.assertEquals(count,3,"the actDatas is :"+count);	 
		}catch(BaseException e){
			Assert.assertTrue(false,"bulkinsert fail "+e.getMessage());								
		}		
	}
	
	/**
	*test bulkInsert (List< BSONObject > insertor, int flag)��set flag=1/-1,ignore flag value 
	*/
	public void bulkInsertFlagError(){
		List<BSONObject>list = new ArrayList<BSONObject>();				
		BSONObject obj = new BasicBSONObject();				
		obj.put("no", 1);				
		list.add(obj);	
		try{
			cl.bulkInsert(list, -1);
			Assert.fail("when flag is -1,it should fail!");
		}catch(BaseException e){
			Assert.assertEquals(e.getErrorCode(), -6,"unexpected error code");	
		}
		
		try{
			cl.bulkInsert(list, 1);				
			long count = cl.getCount();
			Assert.assertEquals(count,4,"the 3th insert actDatas is :"+count);
		}catch(BaseException e){
			Assert.assertTrue(false,"bulkinsertFlag fail "+e.getMessage());	
		}	
	}
	
	@AfterClass(alwaysRun = true)
	public void tearDown(){
		try{
			if(cs.isCollectionExist(clName)){
				cs.dropCollection(clName);
			}				
			sdb.disconnect();
			System.out.println(this.getClass().getName()+" end at "
					+new SimpleDateFormat("yyyy-MM-dd HH:mm:ss:S").format(new Date()));
		}catch(BaseException e){			
			Assert.assertTrue(false,"clean up failed:"+e.getMessage());
		}
	}
	
	@Test
	public void testBulkinsert(){
		try{		
			bulkInsert();
			bulkInsertDuplicateKey();	
			//TODO:bug:SEQUOIADBMAINSTREAM-1989
			bulkInsertFlagError();
		}catch(BaseException e){
		   e.printStackTrace();
		   Assert.assertTrue(false, e.getMessage());	
		}
	}	
}
