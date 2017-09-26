package com.sequoiadb.metaopr.noderestart;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.exception.ReliabilityException;
import org.bson.BSONObject;
import org.bson.util.JSON;

import java.util.ArrayList;
import java.util.List;

public class Utils {
    public static void checkConsistency(GroupWrapper cataGroup) 
    		throws ReliabilityException, InterruptedException {
        cataGroup.refresh();
        List<String> urls = cataGroup.getAllUrls();
        
        int retryTimes = 0;
        int sleepTimes = 300;  //ms
        int maxRetryTimes = 50;
        List<List<BSONObject>> resList = null;
        while (true) {
	        retryTimes++;
	        Thread.sleep(sleepTimes);
	        resList = new ArrayList<List<BSONObject>>();
	        
	        // get catalog info from all catalog nodes
	        for (String url : urls) {
	            Sequoiadb cataDB = new Sequoiadb(url, "", "");
	            DBCollection cl = cataDB.getCollectionSpace("SYSCAT").getCollection("SYSCOLLECTIONS");
	            DBCursor cursor = cl.query(null, null, (BSONObject) JSON.parse("{ _id: 1 }"), null);
	            List<BSONObject> res = new ArrayList<BSONObject>();
	            while (cursor.hasNext()) {
	                res.add(cursor.getNext());
	            }
	            cursor.close();
	            resList.add(res);
	            cataDB.close();
	        }
	        
	        // check catalog count
	        if (resList.get(0).size() == resList.get(1).size() 
	        		&& resList.get(1).size() == resList.get(2).size()) {
	        	break;
	        } else if (retryTimes >= maxRetryTimes) {
	            System.out.println(resList.get(0).size() 
	            		+ " " + resList.get(1).size() 
	            		+ " " + resList.get(2).size());
	            throw new ReliabilityException("Failed to check count between catalog nodes!");
	        }
        }
        
        // check catalog content
        List<BSONObject> srcList = resList.get(0);
        for (int i = 1; i < resList.size(); i++) {
            List<BSONObject> dstList = resList.get(i);
            for (int j = 0; j < srcList.size(); j++) {
                if (!srcList.get(j).equals(dstList.get(j))) {
                    System.out.println(urls.get(0) + " : " + srcList.get(j));
                    System.out.println(urls.get(i) + " : " + dstList.get(j));
    	            throw new ReliabilityException("Failed to check records between catalog nodes!");
                }
            }
        }
        
    }
    
    public static String getKeyStack(Exception e, Object classObj) {
        StringBuffer stackBuffer = new StringBuffer();
        StackTraceElement[] stackElements = e.getStackTrace();
        for (int i = 0; i < stackElements.length; i++) {
            if (stackElements[i].toString().contains(classObj.getClass().getName())) {
                stackBuffer.append(stackElements[i].toString()).append("\r\n");
            }
        }
        String str = stackBuffer.toString();
        if (str.length() >= 2) {
            return str.substring(0, str.length() - 2);
        } else {
            return str;
        }
    }
}
