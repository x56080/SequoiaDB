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

   Source File Name = XmlConfig.java

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

import java.util.ArrayList;

public class XmlConfig {
    private String editionId = "";
    private ArrayList<String> editionValues = new ArrayList<String>();
    private ArrayList<SDBDocument> docs = new ArrayList<SDBDocument>();

	public void addDocument(SDBDocument doc){
		docs.add(doc);
	}	
	public SDBDocument getDocument(int num){
		SDBDocument doc = docs.get(num);
		return doc;
	} 
	public ArrayList<SDBDocument> getDocuments(){
		return docs;
	}
    public void addEditionID(int editionId){
    	if(this.editionId.compareTo("") == 0){
    		this.editionId = String.valueOf(editionId);
    	}else{
    		this.editionId = editionId + "," + this.editionId;
    	}
    }
    public String getEditionID(){
    	return editionId;
    }
	public String getEditionValue(int num) {
		return editionValues.get(num);
	}
	public void addEditionValue(int editionId,String editionValue) {
		editionValues.add(String.valueOf(editionId));
		editionValues.add(editionValue);
	}
	public boolean idExist(int id){
		int flag = editionId.indexOf(String.valueOf(id));
		if(-1 == flag){
			return false;
		}
		return true;
	}
}
