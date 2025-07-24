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

   Source File Name = sdbProperty.java

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

import org.apache.tools.ant.Task;

public class sdbProperty extends Task{
	private String proName ; 
	private String proPort ; 
	private String name;
	private String value;
	
	public void setProName( String value )
	{
		this.proName = value ;
	}
	public void setProPort( String value )
	{
		this.proPort = value ; 
	}
	
	public String getProName()
	{
		return this.proName ; 
	}
	public String getProPort()
	{
		return this.proPort ; 
	}
	public void setName( String value ){
		this.name = value;
	}
	public void setValue( String value ){
		this.value = value;
	}
	public void execute(){
		getProject().setUserProperty(this.name, this.value);
	}

}
