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

   Source File Name = SdbFileAdd.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.ant.tools;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.io.UnsupportedEncodingException;

import org.apache.tools.ant.Task;

public class SdbFileAdd extends Task{
	public String srcFileName = null;
	public String destFileName = null;
	private BufferedReader reader = null;
	private BufferedWriter output = null;
	private String temp = null;
	public void setSrcFile(String value){
		this.srcFileName = value;
	}
	public void setDestFile(String value){
		this.destFileName = value;
	}
	public void execute(){
		
		File srcFile = new File( srcFileName ) ;
		File destFile = new File( destFileName ) ;
		if( ! destFile.exists() ){
			try {
				if( ! destFile.createNewFile() )
					throw new Exception( "create "+destFile+" file fail" ) ;
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			} catch (Exception e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			} 
		}
		if( ! srcFile.exists() ){
			try {
				throw new Exception(srcFileName + " is not exist\n");
			} catch (Exception e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		}
		
//		BufferedWriter output = null;
		try {
			InputStreamReader srcFile_r = new InputStreamReader( new FileInputStream(srcFile),"UTF-8");
			reader = new BufferedReader(srcFile_r);
			output = new  BufferedWriter( new OutputStreamWriter(new FileOutputStream( destFile ,true) ,"UTF-8" )) ; 
		} catch (FileNotFoundException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (UnsupportedEncodingException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} 
		try {
			
			
			while( (temp = reader.readLine()) != null ){
				output.write( temp+"\n" ) ;
				
			}
			reader.close();
			output.close() ;
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		
		
	}
}
