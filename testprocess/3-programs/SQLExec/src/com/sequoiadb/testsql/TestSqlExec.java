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

   Source File Name = TestSqlExec.java

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

import java.sql.SQLException;
import java.sql.Statement;
import java.sql.ResultSet;
import java.sql.Connection;
import java.sql.ResultSetMetaData;

import org.apache.log4j.*;

import com.sequoiadb.util.EXPECTEDFLAG;

import java.util.LinkedList;
import java.util.ArrayList;

public class TestSqlExec implements Runnable {
	
	private Connection conn;
	private Statement stmt;
	private String sqlExceptionMessage;
	private int errCode;
	private String sql;
	private boolean resultReturn;
	private boolean succFlag;
	
	//expected flag
	private EXPECTEDFLAG expectedFlag;
	
	//result set
	private ResultSetMetaData rsMeta;
	private LinkedList<ArrayList<String>> resultSet;
	private int rowCnt;
	
	private Logger logger;
	
	public TestSqlExec(Connection conn, String database){
		this.conn = conn;
		expectedFlag = EXPECTEDFLAG.IGNORE;
		if(database != null || database.trim().length()!= 0)
		{
			setDatabase(database);
			setNLSFormat();
		}
		logger = Logger.getLogger(TestSqlExec.class.getName());
	}
	
	public void run(){
		if(!resultReturn)
			execSqlWithNoResult();
		else
			execSqlWithResult();
	}
	
	public boolean getExecFlag(){
		return succFlag;
	}
	
	public EXPECTEDFLAG getExpectedFlag(){
		return expectedFlag;
	}
	
	public void setExpectedFlag(EXPECTEDFLAG expected){
		expectedFlag = expected;
	}
	
	public int getErrorCode(){
		return errCode;
	}
	
	public String getSqlExceptionMessage(){
		return sqlExceptionMessage;
	}
	
	public void preExecSql(String sql, boolean resultReturn){
		this.sql	=	sql;
		this.resultReturn	=	resultReturn;
		isSelectSql();
		if(this.sql.endsWith(";"))
			this.sql = this.sql.substring(0, sql.length()-1);
	}
	
	public int getRowCount(){
		return rowCnt;
	}
	
	public LinkedList<ArrayList<String>> getResultSet(){
		return resultSet;
	}
	
	public ResultSetMetaData getResultSetMetaData(){
		return rsMeta;
	}
	
	public void closedStmt(){
		if(resultReturn)
		{
			try{
				stmt.close();
			}
			catch(SQLException e){
				logger.debug( e.getStackTrace() +" \n--exception message\n" + e.getLocalizedMessage());
			}
		}
	}
	
	private void execSqlWithResult(){
		try{
			ResultSet rs = null;
			try{
				stmt = conn.createStatement();
				rs = stmt.executeQuery(sql);
				rsMeta = rs.getMetaData();
				succFlag = rsToList(rs);
			}catch(SQLException e){
				errCode = e.getErrorCode();
				sqlExceptionMessage = e.getLocalizedMessage();
				succFlag = false;
			}
			finally{
				if(rs != null)
					rs.close();
			}
		}
		catch(Exception e){
			logger.debug( e.getStackTrace() +" \n--exception message\n" + e.getLocalizedMessage());
		}
	}
	
	private void execSqlWithNoResult(){
		try{
			try{
				stmt = conn.createStatement();
				stmt.execute(sql);
				succFlag = true;
			}catch(SQLException e){
				errCode = e.getErrorCode();
				sqlExceptionMessage = e.getLocalizedMessage();
				succFlag = false;
			}
			finally{
				stmt.close();
			}
		}
		catch(Exception e){
			logger.debug( e.getStackTrace() +" \n--exception message\n" + e.getLocalizedMessage());
		}
	}
	
	private boolean setDatabase(String database){
		sql = "use " + database;
		execSqlWithNoResult();
		return succFlag;
	}
	
	private void setNLSFormat(){
		sql = "alter session set nls_date_format='yyyy-mm-dd hh24:mi:ss'";
		execSqlWithNoResult();
		sql = "alter session set nls_timestamp_format='yyyy-mm-dd hh24:mi:ss'";
		execSqlWithNoResult();
	}
	
	private boolean rsToList(ResultSet rs){
		boolean flag = true;
		
		try{
			if(resultSet == null)
				resultSet = new LinkedList<ArrayList<String>>();
			else
				resultSet.clear();
			
			rowCnt = 0;
			while(rs.next()){
				rowCnt++;
				ArrayList<String> tmp = new ArrayList<String>();
				for(int i=0; i<rsMeta.getColumnCount(); i++){
					//tmp.add(rs.getString(i+1));
					String col = rs.getString(i+1);
					if(col == null || rs.wasNull())
						tmp.add("null");
					else
						tmp.add(col);
				}
				resultSet.add(tmp);
			}
		}
		catch(SQLException e){
			errCode = e.getErrorCode();
			sqlExceptionMessage = e.getLocalizedMessage();
			flag = false;
			logger.debug( e.getStackTrace() +" \n--exception message\n" + e.getLocalizedMessage());
		}
		catch(Exception e){
			sqlExceptionMessage = e.getLocalizedMessage();
			flag = false;
			logger.debug( e.getStackTrace() +" \n--exception message\n" + e.getLocalizedMessage());
		}
		
		return flag;
	}
	
	//is Select Sql?
	private void isSelectSql(){
		String tmp = sql.trim().toUpperCase();
		
		if(tmp.startsWith("INSERT") 
			||tmp.startsWith("CREATE")
			||tmp.startsWith("DROP")
			||tmp.startsWith("UPDATE")
			||tmp.startsWith("DELETE")
			||tmp.startsWith("USE")
			||tmp.startsWith("ALTER"))
		{
			resultReturn = false;
		}
	}
	
}
