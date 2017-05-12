package com.sequoiadb.ant.sdbtask;

import java.util.List;
import com.sequoiadb.base.*;
import org.apache.tools.ant.Task;
import com.sequoiadb.ant.tools.*;

import org.apache.tools.ant.types.Parameter;
/**
 * @author chenzichuan
 */
public class SdbGetNodeInfo extends Task{
	private String hostName ; 
	//private String propertyHostName ;
	//private String propertyNodePort ; 
	private String groupName ; 
	private String getNodeType ; 
	private String getNum = "1" ; 
	private setPropertyInfo setProInfo = null ; 
	private hostNames htNames = null ; 
	

	public void setGetNum( String value )
	{
		this.getNum = value ; 
	}
	public void setHostName( String value )
	{
		this.hostName = value ; 
	}
	public void setGroupName( String value )
	{
		this.groupName = value ; 
	}
	public void setGetNodeType( String value )
	{
		value = value.toLowerCase() ; 
		this.getNodeType = value ; 
	}
	
	public hostNames createHostName()
	{
		this.htNames = new hostNames() ;
		return this.htNames ; 
	}
	
	public setPropertyInfo createSetProperty()
	{ 
		this.setProInfo =  new setPropertyInfo() ;
		return this.setProInfo ; 
	}
	
	public int setProperty( ReplicaGroup group )
	{
		String propertyHostName = group.getMaster().getHostName().toString() ;
		String propertyNodePort = Integer.toString( group.getMaster().getPort() ) ;
		List<sdbProperty> listPro = this.setProInfo.getListPro() ; 
		if( this.getNodeType.equals("master") || ( this.getNodeType.equals("slave") && this.getNum.equals("1") ) )
		{
			if( ! this.getNodeType.equals("master") )
			{
				propertyHostName = group.getSlave().getHostName().toString() ;
				//propertyNodePort = Integer.toString( group.getSlave().getPort() ) ; 
			}
			for( sdbProperty sdbpro : listPro )
			{
				this.getProject().setProperty( sdbpro.getProName() , propertyHostName ) ;
				this.getProject().setProperty( sdbpro.getProPort() , propertyNodePort ) ;
			}
			
			return 0 ; 
		}
		if( ! this.getNodeType.equals("master") && ! this.getNum.equals("1") && this.htNames != null )
		{
			
			List<Parameter> listHtName = this.htNames.getListParameter() ; 
			for( Parameter p : listHtName )
			{
				if ( propertyHostName.equals( p.getValue() ) )
				{
					listHtName.remove( p ) ; 
					break ; 
				}
			}
			int i = 0 ; 
			for( sdbProperty sdbpro : listPro )
			{
				this.getProject().setProperty( sdbpro.getProName() , listHtName.get(i++).getValue() ) ;
				this.getProject().setProperty( sdbpro.getProPort() , propertyNodePort ) ;

			}
		}
		return 0 ; 
		
	}
	
	public void execute()
	{
		Sequoiadb sdb = new Sequoiadb( this.hostName  ,50000 , "" ,"") ;

		ReplicaGroup group = sdb.getReplicaGroup( this.groupName ) ;
		
		this.setProperty( group ) ; 

	}

	
}
