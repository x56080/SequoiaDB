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

   Source File Name = SdbtaskBaseName.java

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

import java.io.File;


import org.apache.tools.ant.Task;

/**
 * @author chenzichuan
 */
public class SdbtaskBaseName extends Task {
	private String file ; 
	private String property ; 
	private String suffix = null ; 
	
	public void setFile( String value )
	{
		this.file = value ; 
	}
	
	public void setProperty( String value )
	{
		this.property = value ; 
	}
	public void setSuffix( String value )
	{
		this.suffix = value ; 
	}
	public void execute()
	{
		File filePath = new File( this.file ) ; 
		String fileName = filePath.getName(); 
		
		if( this.suffix != null ) 
		{
			String[] token = fileName.split( this.suffix ) ;
			this.getProject().setProperty( this.property  , token[0] ) ;
		}
		else{
			this.getProject().setProperty( this.property , fileName ) ; 
		}
		
	}
}
