package com.sequoiadb.ant.tools;


import java.util.ArrayList;
import java.util.List;


public class setPropertyInfo {
	
	private List<sdbProperty> listPro = new ArrayList<sdbProperty>();
	
	public sdbProperty createSdbProperty()
	{
		sdbProperty sdbProperty_one = new sdbProperty() ; 
		listPro.add( sdbProperty_one ) ; 
		return sdbProperty_one ; 
	}
	public List<sdbProperty> getListPro()
	{
		return this.listPro ; 
	}
}
