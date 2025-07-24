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

   Source File Name = WirteSdb.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.tasks;

import java.sql.Connection;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.SQLException;

import com.sequoiadb.service.ConnectDataBase;

public class WirteSdb {
     

	public static void add(String dbType,String url,String user,String password){
		
		Connection conn = null;
		
		PreparedStatement pstmt = null;
		
		
	   ConnectDataBase cdb = new ConnectDataBase(dbType,url,user,password);
		
		conn = cdb.getConnection();
		
		String sql = "insert into sedb8 (column1,column2,column3,column4,column5,column6,column7,column8,column9,column10) values (?,?,?,?,?,?,?,?,?,?)";//sql
		String sql1 = "insert into sedb10(column1,column2,column3,column4,column5,column6,column7,column8,column9,column10) values (?,?,?,?,?,?,?,?,?,?)";
		try {
			   pstmt = conn.prepareStatement(sql1);
			   for(int i=1000000;i<1000050;i++){
			   pstmt.setInt(1,i);
			   pstmt.setString(2, "21zddddddddddxcvbnmlkjhgfdsaqwertppoiuytrewqasdfghjklmnbvcxzqwertyuioplkjhgfdsazxcvbnmlkjhgfdsaqwertyuiop"+i);
			   pstmt.setString(3, "31zsadsfgbhnjddxcvbnmlkjhgfdsaqwertyppoiuytrewqasdfghjklmnbvcxzqwertyuioplkjhgfdsazxcvbnmlkjhgfdsaqwertyuiop"+i);
			   pstmt.setString(4, "41zewrtujkmlioxcvbnmlkjhgfdsaqwertyppoiuytrewqasdfghjklmnbvcxzqwertyuioplkjhgfdsazxcvbnmlkjhgfdsaqwertyuiop"+i);
			   pstmt.setString(5, "51zdvfvgbhnmjxcvbnmlkjhgfdsaqwertyupoiuytrewqasdfghjklmnbvcxzqwertyuioplkjhgfdsazxcvbnmlkjhgfdsaqwertyuiop"+i);
			   pstmt.setString(6, "651zasxcdfvgbnxcvbnmlkjhgfdsaqwertyuiouytrewqasdfghjklmnbvcxzqwertyuioplkjhgfdsazxcvbnmlkjhgfdsaqwertyuiop"+i);
			   pstmt.setString(7, "751znmjkloipytxcvbnmlkjhgfdsaqwertyuioppoiewqasdfghjklmnbvcxzqwertyuioplkjhgfdsazxcvbnmlkjhgfdsaqwertyuiop"+i);
			   pstmt.setString(8, "851zxascvfbghnxcvbnmlkjhgfdsaqwertyuioppoiuytrsdfghjklmnbvcxzqwertyuioplkjhgfdsazxcvbnmlkjhgfdsaqwertyuiop"+i);
			   pstmt.setString(9, "951zxaqsxcdwertcvbnmlkjhgfdsaqwertyuioppoiuytrewqahjklmnbvcxzqwertyuioplkjhgfdsazxcvbnmlkjhgfdsaqwertyuiop"+i);
			   pstmt.setString(10,"hello"+i);
			   pstmt.addBatch();
			   if(i%10 == 0){
				   pstmt.executeBatch();
				   pstmt.clearBatch();
			   }
			   }
			  } catch (SQLException e) {
			   e.printStackTrace();
			  } finally {
			   try {
			    if (pstmt != null)
			     pstmt.close();
			    if (conn != null)
			     conn.close();
			   } catch (SQLException e) {
			    e.printStackTrace();
			   }
		    }
	}	
	public static void main(String[] args) {
		String command = "Sdb import --connect jdbc:db2://192.168.30.223:50110/sequoia --username db2inst1";
	    add("db2","jdbc:db2://192.168.30.223:50110/sequoia","db2inst1","liuck_2015");
	}
}
