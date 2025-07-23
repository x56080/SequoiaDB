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

   Source File Name = OperateConnection.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.testsql;

import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.SQLException;


public class OperateConnection {
	
	public static Connection initConnection(String url, String uid, String pwd){
		try{
			return DriverManager.getConnection(url, uid, pwd);
		}
		catch(Exception e){
			e.printStackTrace();
			return null;
		}
	}
	
	public static void destroyConnection(Connection conn){
		try{
			if(conn != null && !conn.isClosed())
				conn.close();
			return;
		}
		catch(SQLException e){
			e.printStackTrace();
		}
	}
	
	public static void loadThirdDriver(String driverName){
		try{
			Class.forName(driverName);
		}catch(ClassNotFoundException e){
			e.printStackTrace();
		}
	}
}
