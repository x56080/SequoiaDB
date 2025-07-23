/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = CsAndClOperation.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.samples;

/******************************************************************************
 * 
 * Name: CsAndClOperation.java 
 * Description: This program demonstrates how to use the Java Driver to
 * 				operate on collectionSpace and collection
 *				Get more details in API document
 * 
 ******************************************************************************/

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;

public class CsAndClOperation {
	public static void main(String[] args) {
		if (args.length != 1) {
			if (args.length != 1) {
				System.out
						.println("Please give the database server address <IP:Port>");
				System.exit(0);
			}
		}

		/* get connection */
		
		// the database server address, IP:PORT
		String connString = args[0];
		Sequoiadb sdb = null;
		// connect to specifical db, may cause some baseException, e.g. Network error
		try {
			sdb = new Sequoiadb(connString, "", "");
			// or
//			sdb = new Sequoiadb("username", "password");
			// or
//			sdb = new Sequoiadb("IP", port, "username", "password");
		} catch (BaseException e) {
			System.out.println("Failed to connect to database: " + connString + ", error description" + e.getErrorType());
			e.printStackTrace();
			System.exit(1);
		}
		
		/* get collectionSpace */
		
		// create collectionspace, if collectionspace exists get it
		CollectionSpace cs = null;
		if(sdb.isCollectionSpaceExist(Constants.CS_NAME))
			cs = sdb.getCollectionSpace(Constants.CS_NAME);
		else
			cs = sdb.createCollectionSpace(Constants.CS_NAME);
			// or sdb.createCollectionSpace(csName, pageSize), need to specify the pageSize
		
		/* get collection */
		
		// create collection, if collection exists get it
		DBCollection cl = null;
		if(cs.isCollectionExist(Constants.CL_NAME))
			cl = cs.getCollection(Constants.CL_NAME);
		else
			cl = cs.createCollection(Constants.CL_NAME);
			// or cs.createCollection(collectionName, options), create collection with some options
		
		System.out.println("Current collection name: " + cl.getName());
		System.out.println("Current collection fullName: " + cl.getFullName());
		System.out.println("Current collectionSpace name:" + cl.getCSName());

		/*
		 * if you want to drop cs or cl, you can use next two functions:
		 * sdb.dropCollectionSpace(csName)
		 * cs.dropCollection(collectionName)
		 */
		
		
		// then you can do operations(e.g. CRUD) on data using cs and cl handlers...
		
		// do not forget to disconnect from db
		sdb.disconnect();
	}
}
