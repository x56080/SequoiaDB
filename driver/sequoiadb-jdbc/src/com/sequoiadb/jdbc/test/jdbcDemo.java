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

   Source File Name = jdbcDemo.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.jdbc.test;

import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;
import java.sql.Timestamp;
import java.util.Date;

public class jdbcDemo {
public static void main(String[] args) throws SQLException {
	Connection conn = null;
	try {
		Class.forName("com.sequoiadb.jdbc.Driver");
		System.out.println("ɹsequoiadb");
		String url = "jdbc:sequoiadb://192.168.20.45:11810";
		conn = DriverManager.getConnection(url);
		Statement stmt = conn.createStatement();
		ResultSet rs = stmt.executeQuery("select * from foo.bar where T9 = null");
		while(rs.next()){	
//		Double str1 = rs.getDouble("T1");
		Double str2 = rs.getDouble(1);
		int str3 = rs.getInt(3);
		System.out.println(str2+" "+str3);
//		Double str4 = rs.getDouble("T4");
//		Date str15 = rs.getDate("T15");
//		String str16 = rs.getString("T16");
//		String str18 = rs.getString("T18");
//		String str19 = rs.getString("T19");
//		int str20 = rs.getInt("T20");
//		String str21 = rs.getString("T21");
//		System.out.println(" "+str2+" "+str3+" "+str4+" "+str16+"  "+str15+"  "+str18+" "+str21+"  "+str19+"  "+str20);
		}
	} catch (Exception e) {
		e.printStackTrace();
	}
}
}
