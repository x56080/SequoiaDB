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

   Source File Name = ResultSetCompare.java

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

import java.util.LinkedList;
import java.util.ArrayList;

import java.sql.ResultSetMetaData;
import java.sql.Timestamp;
import java.sql.Types;


public class ResultSetCompare {
	
	private final static float floatMin = 1E-6f;
	
	private ResultSetMetaData rsMeta;
	private LinkedList<ArrayList<String>> rs1;
	private LinkedList<ArrayList<String>> rs2;
	
	private StringBuffer debugMessage;
	private boolean orderbyFlag;
	private boolean terminateFlag;
	
	public ResultSetCompare(LinkedList<ArrayList<String>> rs1, LinkedList<ArrayList<String>> rs2, ResultSetMetaData rsMeta, boolean orderby){
		this.rs1 = rs1;
		this.rs2 = rs2;
		this.rsMeta = rsMeta;
		this.orderbyFlag = orderby;
		
		terminateFlag = false;
		
		buildMessage();
	}
	
	public String getMessage(){
		return debugMessage.toString();
	}
	
	public boolean compareWork(){
		boolean flag = false;
		
		ArrayList<String> row1, row2;
		int i, j, colIndex;
		int columnCnt = rs1.getFirst().size();
		int rowCnt = rs1.size();
		
		for(i=0; i<rowCnt; i++)
		{
			row1 = rs1.getFirst();
			
			for(j=0; j<rs2.size(); j++)
			{
				row2 = rs2.get(j);
				
				for(colIndex=0; colIndex<columnCnt; colIndex++)
				{
					flag = recordCompare(colIndex, row1.get(colIndex), row2.get(colIndex));
					if(!flag){	//the columns of tow rows are not equal, terminate the row comparing process, get next row of rs2 to compare
						break;
					}
				}
				
				if(flag){	//rows compared process returns ture, then remove those rows from resultsets, and tenaminate this loop to take next row to compare
					rs1.removeFirst();
					rs2.remove(j);
					break;
				} 
				else if(orderbyFlag){//resultset is in order, but the rows are not match, needn't take next row to compare. terminate the all comparing process
					break;
				}
				else if(terminateFlag){	// the all comparing process terminate
					break;
				}
			}
			
			if(!flag){	//the row of rs1 isn't found a row of rs2 to match, then two ResultSets are compared failed, teminate the all comparing process
				break;
			}
		}

		return flag;
	}
	
	private boolean recordCompare(int colIndex, String data1, String data2){
		boolean flag = true;
		
		
		//first, compare datas with string type,
		//if not equal, compare datas again with datatype
		if(data1.compareTo(data2) != 0)
		{
			if(data1.equalsIgnoreCase("null") || data2.equalsIgnoreCase("null")){	//if not equal and one record is null, then return false
				return false;
			}
		
			flag = compareByDataType(colIndex, data1, data2);
		}
			
		return flag;
	}
	
	//compare datas with data type
	private boolean compareByDataType(int colIndex, String data1, String data2){
		boolean flag = false;
		try{
			switch(rsMeta.getColumnType(colIndex+1)){
				case Types.FLOAT:
				case Types.DOUBLE:
				case Types.NUMERIC:
					double d1 = Double.valueOf(data1);
					double d2 = Double.valueOf(data2);
					
					if(Math.abs(d1-d2) < floatMin || Math.abs(d1-d2)<Math.abs(d1)*floatMin)
						flag = true;
					break;
					
				case Types.INTEGER:
				case Types.BIGINT:
					long long1 = Long.valueOf(data1);
					long long2 = Long.valueOf(data2);
					if(long1 == long2)
						flag = true;
					break;
				
				case Types.VARCHAR:
				case Types.CHAR:
					break;
					
				case Types.DATE:
				case Types.TIME:
				case Types.TIMESTAMP:
					Timestamp ts1 = Timestamp.valueOf(data1);
					Timestamp ts2 = Timestamp.valueOf(data2);
	
					if(ts1.compareTo(ts2)==0)
						flag = true;
					break;
				
				default:
					debugMessage.append("\ndon't deal with datatype " + rsMeta.getColumnTypeName(colIndex+1) + " \n");
					terminateFlag = true;
					flag = false;
					break;
			}
		}catch(Exception e){
			debugMessage.append("\ncolumn1:"+ data1 + "\ncolumn2:" + data2 +"\ncompare datas throw exception \n");
			debugMessage.append(e.getLocalizedMessage());
		}
		
		return flag;
	}
	
	private void buildMessage(){
		debugMessage = new StringBuffer();
		debugMessage.append("--ResultSet1:\n");
		if(rs1.size() > 100)
		{
			debugMessage.append(rs1.size() + " rows are too much to display\n");
		}
		else{
			for(int i=0; i<rs1.size(); i++)
			{
				ArrayList<String> tmp = rs1.get(i);
				
				for(int j=0; j<tmp.size(); j++)
					debugMessage.append(tmp.get(j) + "\t");
				
				debugMessage.append("\n");
			}
		}
		
		debugMessage.append("***************************\n");
		debugMessage.append("--ResultSet2:\n");
		if(rs2.size() > 100){
			debugMessage.append(rs2.size() + " rows are too much to display\n");
		}
		else{
			for(int i=0; i<rs2.size(); i++)
			{
				ArrayList<String> tmp = rs2.get(i);
				
				for(int j=0; j<tmp.size(); j++)
					debugMessage.append(tmp.get(j) + "\t");
				
				debugMessage.append("\n");
			}
		}
	}
}
