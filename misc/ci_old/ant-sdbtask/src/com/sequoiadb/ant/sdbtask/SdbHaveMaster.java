package com.sequoiadb.ant.sdbtask;


import org.apache.tools.ant.Task;
import org.bson.BSONObject;
import org.bson.types.BasicBSONList;

import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
/**
 * @author 
 */
public class SdbHaveMaster extends Task {
	private String hostName = "localhost" ;
	private String port = "50000" ; 
	private String groupName = null ; 
	private String waitTime = "120" ; 
	private String propertyName= null ;
	private static int ERROR_STANTALONG=-159;
	private boolean failonerror = true;
	
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
		if( value.equals("1") )
			this.groupName = "SYSCatalogGroup";
		else
			this.groupName = value ;
	}
	public void setWaitTime( String value )
	{
		this.waitTime = value ; 
	}
	public void setFailonerror( boolean value ){
		this.failonerror = value;
/*		failonerror = Boolean.parseBoolean(value);*/
	}
	
	private boolean checkMaster( ReplicaGroup RG , Sequoiadb sdb )
	{
		
			
			//GroupID  ;
			String groupID = RG.getDetail().get( "GroupID" ).toString() ;
			int nodeNum = RG.getNodeNum(null) ; 
			//System.out.println( RG.getNodeNum(null) ) ;
			BasicBSONList bson_list = (BasicBSONList)RG.getDetail().get("Group") ;
			
			for(int i = 0 ; i < nodeNum ; i++ )
			{
				try{
//					BSONObject oneBson = (BSONObject) bson_list.get( i ) ; 
//					String nodeID = oneBson.get( "NodeID" ).toString() ;
//					if( sdb.getSnapshot(7,"{GroupID:"
//							+ groupID + ",NodeID:"
//							+ nodeID + "}"
//							, "{\"IsPrimary\":null}"
//							, null).hasNext() )
//					{
//						String isMaster = sdb.getSnapshot(7,"{GroupID:"
//									+ groupID + ",NodeID:"
//									+ nodeID + "}"
//									, "{\"IsPrimary\":null}"
//									, null)
//									.getNext().get("IsPrimary").toString() ;
//						if( isMaster.equals( "true" ) )
//							return true ;
//					}
					BSONObject nodeBson = (BSONObject) bson_list.get( i ) ; 
					//String nodeID = nodeBson.get( "NodeID" ).toString() ;
					BasicBSONList nodeService_list = (BasicBSONList)nodeBson.get("Service");
					//System.out.println(nodeService_list);
					BSONObject nodePort_bson = (BSONObject) nodeService_list.get(0);
					int nodePort = Integer.parseInt((String) nodePort_bson.get("Name"));
					//System.out.println(nodePort);
					String nodeHN = (String) nodeBson.get("HostName"); 
//					System.out.println(nodeHN);
					if(nodeHN.equals("suse-test2"))
						nodeHN="192.168.20.191";
					if(nodeHN.equals("suse-test3"))
						nodeHN="192.168.20.192";
					if(nodeHN.equals("suse-test4"))
						nodeHN="192.168.20.193";
					
					if(nodeHN.equals("ubun-test2"))
						nodeHN="192.168.20.201";
					if(nodeHN.equals("suse-test3"))
						nodeHN="192.168.20.202";
					if(nodeHN.equals("suse-test4"))
						nodeHN="192.168.20.203";
					Sequoiadb nodedb = new Sequoiadb(nodeHN, nodePort,"","");
					DBCursor cur = nodedb.getSnapshot(6, "{}",
							"{\"IsPrimary\":null}",null); 
					if(cur.hasNext()){
						String isMaster = cur.getNext().get("IsPrimary").toString();
						
					
					System.out.println("isMaster is \""+isMaster+"\"");
						if( isMaster.equals( "true" ) ){
							System.out.println("true");
							return true ;
						}
					}
				}catch( BaseException e ){}
				
			}
		
		
	//	
		return false; 
	}
	
	public void execute (){
		Sequoiadb sdb = null;
		boolean isError = false;
		BaseException errorSdb = null;
		try{
			sdb = new Sequoiadb( this.hostName , Integer.parseInt( this.port ) , "" ,"") ;
		}catch( BaseException e ){
			if( failonerror ){
				throw e;
			}
			errorSdb = e;
			isError = true;
		}
		if( !isError ){
			ReplicaGroup RG = null ;
			int i = 0 ;
			int times = Integer.parseInt( this.waitTime ) ;
			for(; i < times ; ++i ){
				try{
					if( ! this.groupName.equals("1") )
						RG = sdb.getReplicaGroup( this.groupName ) ;
					else
						RG = sdb.getReplicaGroup(1) ;
					if( RG != null )
						break;
				}catch( BaseException e ){ 
					try {
						if( e.getErrorCode() == ERROR_STANTALONG )
							this.getProject().setProperty( this.propertyName , "true" ) ;
					   Thread.sleep(1000) ;
				    } catch (InterruptedException e1) {
					   // TODO Auto-generated catch block
					   e1.printStackTrace();
				    }
				}
			}
		
			for(; i < times ; ++i ){
				if( false == checkMaster( RG , sdb ) )
					try {
						Thread.sleep(1000);
					} catch (InterruptedException e) {
						e.printStackTrace();
					}else{	
						System.out.println("is primary");
						this.getProject().setProperty( this.propertyName , "true" ) ;
						break ;
					}
			}
		
			if( ! (i < times) ){
				System.out.println("is not primary");
				this.getProject().setProperty( this.propertyName , "false" ) ;
			}
		}else{
			errorSdb.printStackTrace();
			this.getProject().setProperty( this.propertyName , "false" ) ;
		}
	}
	
}
