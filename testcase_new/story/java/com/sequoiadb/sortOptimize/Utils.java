package com.sequoiadb.sortOptimize;

import java.util.ArrayList; 
import java.util.List;
import java.util.Random;

import org.bson.BSONObject;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;

public class Utils {

	public static boolean  checkSortResult(DBCollection cl, BSONObject sortObj, String sortKey, String className) throws BaseException{
		System.out.println("--------" + className + " begin to check sort result---------");
		DBCursor queryCursor = null;

		try {
			queryCursor = cl.query(null, null, sortObj, null);
			while(queryCursor.hasNext()) {
				String expectStr = (String)queryCursor.getNext().get(sortKey);  // the front one
                                if(queryCursor.hasNext()){
                                    String actStr = (String)queryCursor.getNext().get(sortKey); // the latter one
                                    if(expectStr.compareTo(actStr) > 0) { 
                                         System.out.println("actResult: " + actStr  + ", expectResult: " + expectStr);
                                         return false;
                                    }
                                }else{
                                    break;
                                }
		        }
			return true;
		}catch(BaseException e){
			e.printStackTrace();
			return false;
		}finally {
			System.out.println("--------" + className + " end to check sort result---------");
			queryCursor.close();
		}
	}

        public static String getRandomString(int length){
                String str = "zxcvbnmlkjhgfdsaqwertyuiopQWERTYUIOPASDFGHJKLZXCVBNM1234567890$%!@";
                Random random = new Random();
                StringBuffer sb = new StringBuffer();
                for(int i = 0; i < length; ++i){
                       int number = random.nextInt(66);
                       sb.append(str.charAt(number));
                }
                return sb.toString();
        } 

}
