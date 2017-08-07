package com.sequoiadb.cappedCL;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;

import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.exception.ReliabilityException;

public class Utils {

	/**
	* check whether all data nodes is consistent
	*/
	public static void checkConsistency(GroupWrapper dataGroup) throws ReliabilityException{
	    dataGroup.refresh();
        List<String> urls = dataGroup.getAllUrls();
        List<List<BSONObject>> resultList = new ArrayList<List<BSONObject>>();
        // get datagroup info from all data nodes
        for (String url : urls) {
            Sequoiadb dataDB = new Sequoiadb(url, "", "");
            DBCursor cursor = dataDB.listCollections();
            List<BSONObject> eachNodeCLNameList = new ArrayList<BSONObject>();
            while (cursor.hasNext()) {
            	eachNodeCLNameList.add(cursor.getNext());
            }
            cursor.close();
            resultList.add(eachNodeCLNameList);
            dataDB.close();
        }
        // check data count
        if (resultList.get(0).size() != resultList.get(1).size() || resultList.get(1).size() != resultList.get(2).size()) {
            System.out.println("data count is different between data nodes! ");
            System.out.println(resultList.get(0).size() + " " + resultList.get(1).size() + " " + resultList.get(2).size());
            throw new ReliabilityException("checkConsistency failed. ");
        }
        // check data nodes content
        List<BSONObject> srcList = resultList.get(0);
        for (int i = 1; i < resultList.size(); i++) {
            List<BSONObject> destList = resultList.get(i);
            for (int j = 0; j < srcList.size(); j++) {
                if (!srcList.get(j).equals(destList.get(j))) {
                    System.out.println("records below are different! ");
                    System.out.println(urls.get(0) + " : " + srcList.get(j));
                    System.out.println(urls.get(i) + " : " + destList.get(j));
                    throw new ReliabilityException("checkConsistency failed. ");
                }
            }
        }
    }
}
