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

   Source File Name = Snapshot.java

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
 * Name: Snapshot.java 
 * Description: This program demonstrates how to use the Java Driver to get
 * 				database snapshot ( for other types of * snapshots/lists,
 *				the steps are very similar )
 *				Get more details in API document
 * 
 ******************************************************************************/
import java.io.IOException;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;

import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;

public class Snapshot {
	public static void main(String[] args) throws IOException {
		if (args.length != 1) {
			if (args.length != 1) {
				System.out
						.println("Please give the database server address <IP:Port>");
				System.exit(0);
			}
		}

		// the database server address
		String connString = args[0];
		Sequoiadb sdb = null;
		DBCursor cursor = null;

		try {
			sdb = new Sequoiadb(connString, "", "");
		} catch (BaseException e) {
			System.out.println("Failed to connect to database: " + connString
					+ ", error description" + e.getErrorType());
			e.printStackTrace();
			System.exit(1);
		}
		
		cursor = sdb.getSnapshot(Sequoiadb.SDB_SNAP_COLLECTIONS,
				new BasicBSONObject(), null, null);
		try {
			while (cursor.hasNext())
				System.out.println(cursor.getNext());
		} finally {
			cursor.close();
		}
		sdb.disconnect();
	}
}
