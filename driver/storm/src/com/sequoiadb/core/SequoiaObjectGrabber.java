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

   Source File Name = SequoiaObjectGrabber.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.core;

import java.io.Serializable;
import java.util.List;

import org.bson.BSONObject;

//The mapper class for maps the bson object to tuple list
public abstract class SequoiaObjectGrabber implements Serializable {

	/**
	 * 
	 */
	private static final long serialVersionUID = -1384658504027624352L;

	//maps the bson object to List
	public abstract List<Object> map(BSONObject object);
	
	//Get the tuple's fileds name list.
	public abstract String[] fields();
}
