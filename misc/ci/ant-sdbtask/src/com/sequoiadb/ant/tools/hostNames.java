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

   Source File Name = hostNames.java

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


import java.util.ArrayList;
import java.util.List;

import org.apache.tools.ant.types.Parameter;


public class hostNames {
	
      private List<Parameter> listParam = new ArrayList<Parameter>();

	
	public Parameter createParam()
	{
		Parameter param = new Parameter() ; 
		listParam.add( param ) ; 
		return param ; 
	}
	public List<Parameter> getListParameter()
	{
		return this.listParam ; 
	}

}
