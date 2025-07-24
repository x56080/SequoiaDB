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

   Source File Name = IConnectStrategy.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.datasource;

import java.util.List;


enum Operation {
	GET,
	DELETE
}

enum ItemStatus {
	IDLE,
	USED
}

interface IConnectStrategy {
	public void init(List<String> addresses, List<Pair> _idleConnPairs, List<Pair> _usedConnPairs);
	public ConnItem pollConnItem(Operation opr);
	public String getAddress();
	/*
	 ItemStatus  change  meaning
	   IDLE      >0    one connection had been add to idle pool, 
	                   strategy need to record the info of that idle connection.
	   IDLE      <0    one connection had been removed from idle pool, 
	                   strategy need to remove the info of that idle connection
	   USED      >0    one connection was filled to used pool, 
	                   strategy need to increase amount of used connection with specified address
	   USED      <0    one connection was got out from the used pool, 
	                   strategy need to decrease amount of used connection with specified address
	 */
	public void update(ItemStatus type, ConnItem item, int change);
	public void addAddress(String addr);
	public List<ConnItem> removeAddress(String addr);
}
