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

   
*******************************************************************************/
package com.sequoiadb.ant.datatype;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

//import org.apache.tools.ant.types.*;

public class Node {
	private String host;
	private String baseport;
	//private String catalogaddr;
	private String groupname;
	private String dbpath;
	
	List<ConfItem> confItems = new ArrayList<ConfItem>();
	
	public void setDbpath(String value)
	{
		dbpath = value;
	}
	
	public String getDbpath()
	{
		return dbpath;
	}
	
	public void setHost(String value)
	{
		host = value;
	}
	
	public String getHost()
	{
		return host;
	}
	
	public void setBaseport(String value)
	{
		baseport = value;
	}
	
	public int getBasePort()
	{
		return Integer.parseInt(this.baseport);
	}
	
	//public void setCatalogaddr(String value)
	//{
	//	catalogaddr = value;
	//}
	
	//public String getCatalogaddr()
	//{
	//	return catalogaddr;
	//}
	
	public void setGroupname(String value)
	{
		groupname = value;
	}
	
	public String getGroupName()
	{
		return groupname;
	}
	
	public ConfItem createConfitem()
	{
		ConfItem item = new ConfItem();
		confItems.add(item);
		return item;
	}
	
	public Map<String, String> getConfigMap()
	{
		Map<String, String> configItemsMap = new HashMap<String,String>();
		
		for (ConfItem item : confItems)
		{
			configItemsMap.put(item.getName(), item.getValue());
		}
		return configItemsMap;
	}
	
	public String toString()
	{
		String msg = "{dataNode{";
		msg += "baseport:" + baseport + ",";
		//msg += "catalogaddr:" + catalogaddr + ",";
		msg += "groupname:" + groupname + ",";
		msg += "configtimes{";
		
		for(ConfItem item: confItems)
		{
			msg += item.toString() + ",";
		}
		
		msg += "}";
		
		return msg;
	}
	
}
