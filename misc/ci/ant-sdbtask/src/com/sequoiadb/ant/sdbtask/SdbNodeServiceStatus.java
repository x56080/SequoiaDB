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

   Source File Name = SdbNodeServiceStatus.java

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
import org.bson.BSONObject;
import org.bson.types.BasicBSONList;

import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;

public class SdbNodeServiceStatus extends Task {
	private String hostName = "localhost" ;
	private String port = "50000" ; 
	private String groupName = null ; 
	private String waitTime = "120" ; 
	private String propertyName= null ;
	
	public void setPropertyName( String value )
	{
		this.propertyName = value ;
	}
	public void setPort( String value )
	{
		this.port = value ; 
	}
	public void setHostName( String value )
	{
		this.hostName = value ; 
	}
	public void setGroupName( String value )
	{
		this.groupName = value ;
	}
	public void setWaitTime( String value )
	{
		this.waitTime = value ; 
	}
	
	private boolean checkNodeServiceStatus( ReplicaGroup RG , Sequoiadb sdb )
	{
			//GroupID  ;
			String groupID = RG.getDetail().get( "GroupID" ).toString() ;
			int nodeNum = RG.getNodeNum(null) ; 
			BasicBSONList bson_list = (BasicBSONList)RG.getDetail().get("Group") ;
			int trueNum = 0;
			for(int i = 0 ; i < nodeNum ; i++ )
			{
				String isServiceStatus = null;
				try{
					BSONObject oneBson = (BSONObject) bson_list.get( i ) ; 
					String nodeID = oneBson.get( "NodeID" ).toString() ;
					if( sdb.getSnapshot(7,"{GroupID:"
							+ groupID + ",NodeID:"
							+ nodeID + "}"
							, "{\"ServiceStatus\":null}"
							, null).hasNext() )
						{
							isServiceStatus = sdb.getSnapshot(7,"{GroupID:"
										+ groupID + ",NodeID:"
										+ nodeID + "}"
										, "{\"ServiceStatus\":null}"
										, null)
										.getNext().get("ServiceStatus").toString() ;
							if( isServiceStatus.equals("true"))
								trueNum++;
						}
					}catch( BaseException e){}
				}
			if( trueNum == 3 )
				return true;
			else
				return false;
		}
	
	public void execute ()
	{
		Sequoiadb sdb = new Sequoiadb( this.hostName , Integer.parseInt( this.port ) , "" ,"") ;
		ReplicaGroup RG = null ;
		if( ! this.groupName.equals("1") )
			RG = sdb.getReplicaGroup( this.groupName ) ;
		else
			RG = sdb.getReplicaGroup(1) ;
		
		int times = Integer.parseInt( this.waitTime ) ;
		int i = 0 ;
		for(; i < times ; ++i )
		{
			if( false == checkNodeServiceStatus( RG , sdb ) )
				try {
					Thread.sleep(1000);
				} catch (InterruptedException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
			else
			{	
				this.getProject().setProperty( this.propertyName , "true" ) ;
				break ;
			}
		}
		if( ! (i < times) )
			this.getProject().setProperty( this.propertyName , "false" ) ;
	}

}

