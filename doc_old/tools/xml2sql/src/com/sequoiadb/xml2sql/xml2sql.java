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

   Source File Name = xml2sql.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.xml2sql;

import java.sql.DriverManager;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.text.ParseException;
import java.util.ArrayList;
import java.util.List;

public class xml2sql {
	private static XmlConfig xmlconfig = XmlConfigReader.getInstance().getXmlConfig();
	public static void main(String[] args ) throws ParseException {    
	   List<String> sqls = new ArrayList<String>();
	   sqls.add(DbUtil.queryEditionSQL());
	   sqls.add(DbUtil.insertEditionSQL());
	   sqls.add(DbUtil.updateEditionSQL());
	   sqls.add(DbUtil.queryContentSQL());
	   sqls.add(DbUtil.insertContentSQL());
	   sqls.add(DbUtil.updateContentSQL());
	   sqls.add(DbUtil.queryFileSQL());
	   sqls.add(DbUtil.insertFileSQL());
	   sqls.add(DbUtil.updateFileSQL());
	   LoadDriver_Cn(getConnection("../../../doc/tools/xml2sql/db_cn.properties"),sqls);
//	   LoadDriver_En(getConnection("./db_en.properties"),sqls);
	   System.out.println("Database Update Success!");
    }
	
	public static java.sql.Connection getConnection(String confPath) {
		java.sql.Connection conn = null;	
	    try {  
	        Configuration conf = new Configuration();
	        String hostname = conf.getValue(confPath, "hostname");
	        String database = conf.getValue(confPath, "database");
	        String username = conf.getValue(confPath, "username");
	        String password = conf.getValue(confPath, "password");
	        String encoding = conf.getValue(confPath, "encoding");
	        conf.clear();
	        
	        Class.forName("com.mysql.jdbc.Driver").newInstance();        
	        conn = DriverManager.getConnection("jdbc:mysql://" 
												+ hostname
												+ "/"
												+ database
												+ "?user="
												+ username
												+ "&password=" 
												+ password
												+ "&characterEncoding="
												+ encoding);
	    } catch (SQLException ex) {
	    	System.err.println("SQLException: " + ex.getMessage());
		    System.err.println("SQLState: " + ex.getSQLState());
		    System.err.println("VendorError: " + ex.getErrorCode());
	        ex.printStackTrace();  
	    } catch (Exception ex) {
        	System.err.println("handle the error.");
        	ex.printStackTrace();
        } 
	    return conn;  
	}  
	
	public static void LoadDriver_Cn(java.sql.Connection conn,List<String> sqls){
		
		try {
			runEditionSQL(conn,sqls);
			runContentCnSQL(conn,sqls);
			runFileCnSQL(conn,sqls);	
		} catch (SQLException ex) {
			System.err.println("SQLException: " + ex.getMessage());
		    System.err.println("SQLState: " + ex.getSQLState());
		    System.err.println("VendorError: " + ex.getErrorCode());
			ex.printStackTrace();
		} finally {
			try {
				conn.close();
			} catch (SQLException e) {
				System.err.println("SQLException: " + e.getMessage());
				e.printStackTrace();
			}
		}     
	}
	
	public static void runEditionSQL(java.sql.Connection conn,List<String> sqls) throws SQLException {           
		for(int i=0;i<xmlconfig.getDocuments().size();i++){
			if(0 == queryEditionSQL(conn,sqls.get(0))){
				insertEditionSQL(conn,sqls.get(1));
			}else if(0 < queryEditionSQL(conn,sqls.get(0))){
				updateEditionSQL(conn,sqls.get(2));
			}else{
				System.out.println("There is a wrong edition record.");
				continue;
			}
        }  
    } 
	
	public static void runContentCnSQL(java.sql.Connection conn,List<String> sqls) throws SQLException{
		for(int i=0;i<xmlconfig.getDocuments().size();i++){
			if(0 == queryContentSQL(conn,i,sqls.get(3))){
				insertContentCnSQL(conn,i,sqls.get(4));
			}else if(0 < queryContentSQL(conn,i,sqls.get(3))){
				updateContentCnSQL(conn,i,sqls.get(5));
			}else{
				System.out.println("There is a wrong content record.");
				continue;
			} 
        }
	}
	
	public static void runFileCnSQL(java.sql.Connection conn,List<String> sqls) throws SQLException{
		for(int i=0;i<xmlconfig.getDocuments().size();i++){
			if(0 == queryFileSQL(conn,i,sqls.get(6))){
				insertFileCnSQL(conn,i,sqls.get(7));
			}else if(0 < queryFileSQL(conn,i,sqls.get(6))){
				updateFileCnSQL(conn,i,sqls.get(8));
			}else{
				System.out.println("There is a wrong file record.");
				continue;
			}
        }
	}
	
	public static int queryEditionSQL(java.sql.Connection conn,String sql) throws SQLException{
		int row = 0;
		java.sql.PreparedStatement pstmt = conn.prepareStatement(sql);
		pstmt.setInt(1,Integer.parseInt(xmlconfig.getEditionValue(0)));
		ResultSet result = pstmt.executeQuery();
		result.last();
		row = result.getRow();
		pstmt.close();
		return row;
	}
	
	public static void insertEditionSQL(java.sql.Connection conn,String sql) throws SQLException{
		int date = (int) (System.currentTimeMillis()/1000);
		java.sql.PreparedStatement pstmt = conn.prepareStatement(sql);
		pstmt.setInt(1,Integer.parseInt(xmlconfig.getEditionValue(0)));
        pstmt.setString(2,xmlconfig.getEditionID());
        pstmt.setString(3,xmlconfig.getEditionValue(1));
        pstmt.setString(4,xmlconfig.getEditionValue(1));
        pstmt.setInt(5,date);	        
        pstmt.addBatch();
        pstmt.executeBatch();
        pstmt.close();  
	}
	
	public static void updateEditionSQL(java.sql.Connection conn,String sql) throws SQLException{
		int date = (int) (System.currentTimeMillis()/1000);
		java.sql.PreparedStatement pstmt = conn.prepareStatement(sql);
		pstmt.setInt(1,Integer.parseInt(xmlconfig.getEditionValue(0)));
        pstmt.setString(2,xmlconfig.getEditionID());
        pstmt.setString(3,xmlconfig.getEditionValue(1));
        pstmt.setString(4,xmlconfig.getEditionValue(1));
        pstmt.setInt(5,date);
        pstmt.setInt(6,Integer.parseInt(xmlconfig.getEditionValue(0)));
        pstmt.executeUpdate();
        pstmt.close();  
	}
	
	public static int queryContentSQL(java.sql.Connection conn,int num,String sql) throws SQLException{
		int row = 0;
		java.sql.PreparedStatement pstmt = conn.prepareStatement(sql);
		pstmt.setInt(1,xmlconfig.getDocument(num).getId());
		ResultSet result = pstmt.executeQuery();
		result.last();
		row = result.getRow();
		pstmt.close();
		return row;
	}
	
	public static void insertContentCnSQL(java.sql.Connection conn,int num,String sql) throws SQLException{
		short valid = 1;		
		if(false == xmlconfig.getDocument(num).isValid()){
			valid = 0;
		}
		if(xmlconfig.getDocument(num).isDirectory() == false){
			java.sql.PreparedStatement pstmt = conn.prepareStatement(sql);
			pstmt.setInt(1,xmlconfig.getDocument(num).getId());
	        pstmt.setString(2,xmlconfig.getDocument(num).getCnname());
	        pstmt.setByte(3,(byte)xmlconfig.getDocument(num).getOrder());
	        pstmt.setInt(4,xmlconfig.getDocument(num).getPid());
	        pstmt.setInt(5,valid);		        
	        pstmt.addBatch();
	        pstmt.executeBatch();
	        pstmt.close();  
		}
	}
	
	public static void updateContentCnSQL(java.sql.Connection conn,int num,String sql) throws SQLException{
		short valid = 1;
		if(false == xmlconfig.getDocument(num).isValid()){
			valid = 0;
		}
		if(xmlconfig.getDocument(num).isDirectory() == false){
			java.sql.PreparedStatement pstmt = conn.prepareStatement(sql);
			pstmt.setInt(1,xmlconfig.getDocument(num).getId());
	        pstmt.setString(2,xmlconfig.getDocument(num).getCnname());
	        pstmt.setByte(3,(byte)xmlconfig.getDocument(num).getOrder());
	        pstmt.setInt(4,xmlconfig.getDocument(num).getPid());
	        pstmt.setInt(5,valid);
	        pstmt.setInt(6,xmlconfig.getDocument(num).getId());
	        pstmt.executeUpdate();
	        pstmt.close();  
		}
	}
	
	public static int queryFileSQL(java.sql.Connection conn,int num,String sql) throws SQLException{
		int row = 0;
		java.sql.PreparedStatement pstmt = conn.prepareStatement(sql);
		pstmt.setInt(1,xmlconfig.getDocument(num).getId());
		pstmt.setInt(2,Integer.parseInt(xmlconfig.getEditionValue(0)));	
		ResultSet result = pstmt.executeQuery();
		result.last();
		row = result.getRow();
		pstmt.close();
		return row;
	}
	
	public static void insertFileCnSQL(java.sql.Connection conn,int num,String sql) throws SQLException{
		short valid = 1;
		int date = (int) (System.currentTimeMillis()/1000);
		if(false == xmlconfig.getDocument(num).isValid()){
			valid = 0;
		}
		if(xmlconfig.getDocument(num).isDirectory() == false){
			java.sql.PreparedStatement pstmt = conn.prepareStatement(sql);
	        pstmt.setInt(1,xmlconfig.getDocument(num).getId());
	        pstmt.setString(2,xmlconfig.getDocument(num).getCnname());
	        pstmt.setString(3,xmlconfig.getDocument(num).getCnpath());
	        pstmt.setShort(4,valid);
	        pstmt.setInt(5,date);
	        pstmt.setInt(6,xmlconfig.getDocument(num).getOrder());
	        pstmt.setInt(7,Integer.parseInt(xmlconfig.getEditionValue(0)));		        
	        pstmt.addBatch();
	        pstmt.executeBatch();
	        pstmt.close();  
		}
	}
	
	public static void updateFileCnSQL(java.sql.Connection conn,int num,String sql) throws SQLException{
		short valid = 1;
		int date = (int) (System.currentTimeMillis()/1000);
		if(false == xmlconfig.getDocument(num).isValid()){
			valid = 0;
		}
		if(xmlconfig.getDocument(num).isDirectory() == false){
			java.sql.PreparedStatement pstmt = conn.prepareStatement(sql);
	        pstmt.setInt(1,xmlconfig.getDocument(num).getId());
	        pstmt.setString(2,xmlconfig.getDocument(num).getCnname());
	        pstmt.setString(3,xmlconfig.getDocument(num).getCnpath());
	        pstmt.setShort(4,valid);
	        pstmt.setInt(5,date);
	        pstmt.setInt(6,xmlconfig.getDocument(num).getOrder());
	        pstmt.setInt(7,Integer.parseInt(xmlconfig.getEditionValue(0)));	
	        pstmt.setInt(8,xmlconfig.getDocument(num).getId());
	        pstmt.setInt(9,Integer.parseInt(xmlconfig.getEditionValue(0)));	
	        pstmt.executeUpdate();
	        pstmt.close();  
		}
	}
}
