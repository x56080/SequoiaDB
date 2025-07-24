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

   Source File Name = clearEnvCS.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.ant.sdbtask;

import org.apache.tools.ant.Task;

import com.sequoiadb.base.*;
import com.sequoiadb.exception.BaseException;
public class clearEnvCS extends Task {
	private String hostName;
	private String csprefix;
	private int port = 50000;
	
	public void setHostName( String value )
	{
		this.hostName = value ;
	}
	public void setCsprefix( String value )
	{
		this.csprefix = value;
	}
	public void setPort( String value )
	{
		this.port = Integer.parseInt( value );
	}
	
	public void execute(){
		try{
			Sequoiadb sdb = new Sequoiadb( this.hostName , this.port ,"" , "");
			DBCursor cur = sdb.listCollectionSpaces();
			String t_cs = null;
			log("one test fail , will drop the cs , cspre is "+this.csprefix);
			while( cur.hasNext() ){
				t_cs = cur.getNext().get("Name").toString();
				//System.out.println(t_cs);
				if( t_cs.contains(this.csprefix) ){
					log("will be droped cs's name is "+ t_cs);
					sdb.dropCollectionSpace( t_cs );
					//break;
				}
			}
		}catch( BaseException e ){
	    	System.out.println(e.getMessage());
	    }
	}
	
	
	

}
