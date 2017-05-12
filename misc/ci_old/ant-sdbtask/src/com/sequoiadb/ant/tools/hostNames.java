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
