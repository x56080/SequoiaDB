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

   Source File Name = FileList.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.util;

import java.io.IOException;
import java.io.File;
import java.util.ArrayList;
import java.util.Collections;
import java.util.LinkedList;
import java.util.Random;

/*
 * author: dps
 * testcase dir
 */

public class FileList {
		private ArrayList Filelist = null;
		private ArrayList Dirlist 	= null;
		private String fileArg = null;
		private Random random;
		private boolean caseRandom;
		public boolean SwitchDirFlag = false;
		
	public FileList(String filePath, boolean caseRandom) throws IOException {
		Filelist	=	new ArrayList();
		Dirlist		=	new ArrayList();
		
		fileArg		= filePath;
		File file 	=	new File(filePath);
		random = new Random(System.currentTimeMillis());
		this.caseRandom = caseRandom;
		
		if(file.isDirectory())
		{
			Dirlist = new ArrayList();
			Dirlist.add(file);
			Collections.sort(Dirlist);
		}
		else
		{
			Filelist.add(file);
			Collections.sort(Filelist);
		}
		SwitchDirFlag = false;
	}
	
// ȡһĿ¼Ĵ
// 
//	initFilelistFlagboolean  Ϊtrueʾfilelist
	public boolean getNextDir(boolean initFilelistFlag){
		File tmp	=	null;
		File file[];
		int i;
		
		// ҪʼFilelistôFilelist
		if(initFilelistFlag)
			Filelist.clear();
		
		do{
		// DirlistΪգôֱӷ
			if(Dirlist.isEmpty())
				return false;
			tmp = (File) Dirlist.remove(caseRandom ? random.nextInt(Dirlist.size()) : 0);
//			System.out.println(tmp.getName());
		}while(tmp.getAbsolutePath().toLowerCase().endsWith(".svn"));	//ignore the .svn dir
			
		
		file = tmp.listFiles();
		for(i = 0; i < file.length; i++)
		{
			if(!file[i].isDirectory())
			{
				Filelist.add(file[i]);
				file[i] = null;
			}
		}
		for(i = file.length - 1; i >= 0; i--)
		{
			if(file[i] != null)
				Dirlist.add(file[i]);
		}
		
		SwitchDirFlag = true;
		
		Collections.sort(Filelist);
		
		return true;
	}
	
//	 ȡһļûļ򷵻null
	public File getNextFile(){
		File tmp	=	null;
		
		while(Filelist.isEmpty()&&!DirlistIsEmp())
			getNextDir(false);
		
		if(!Filelist.isEmpty())
			tmp = (File) Filelist.remove(caseRandom ? random.nextInt(Filelist.size()) : 0);
		
		return tmp;
	}
	
//	 ӡļб
	public void showfiles(){
		File tmp	= null;
		
		do
		{
			tmp	= getNextFile();
			if(tmp == null)
				break;
			System.out.println(tmp.getAbsolutePath());
		}while(true);
	}
	
//	ļбǷΪ
	public boolean FilelistIsEmp() {
		return Filelist.isEmpty();
	}

//  Ŀ¼бǷΪ
	public boolean DirlistIsEmp() {
		return Dirlist.isEmpty();
	}
}