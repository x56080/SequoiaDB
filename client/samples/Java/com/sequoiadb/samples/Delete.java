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

   Source File Name = Delete.java

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
 * Name: Delete.java 
 * Description: This program demonstrates how to use the Java Driver to
 * 				delete data in DB
 *				Get more details in API document
 * 
 ******************************************************************************/

import org.bson.BSONObject;
import org.bson.BasicBSONObject;

import com.sequoiadb.base.result.DeleteResult;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;

public class Delete {
	public static void main(String[] args) {
		if (args.length != 1) {
			System.out
					.println("Please give the database server address <IP:Port>");
			System.exit(1);
		}

		// the database server address
		String connString = args[0];
		Sequoiadb sdb = new Sequoiadb(connString, "", "");
		try {
			DBCollection cl = Constants.getCL(sdb);

			BSONObject matcher = new BasicBSONObject();
			matcher.put("Id", 0 );

			DeleteResult result = cl.deleteRecords(matcher);
			System.out.println(result);
		} catch (BaseException e) {
			e.printStackTrace();
		} finally {
			sdb.close();
		}
	}

}
