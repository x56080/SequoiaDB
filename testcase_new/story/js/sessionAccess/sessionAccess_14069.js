/* *****************************************************************************
@discretion: createNode,check parameter instatceid
@author：2018-11-20 wangkexin
***************************************************************************** */

main();

function main()
{	  
	try
	{	
      var db = new Sdb(COORDHOSTNAME, COORDSVCNAME ) ;  
      var groupName = "group14069"; 
	  var rg = db.createRG(groupName);
	  // effective parameters
	  var instanceidList = [ 12, 0, 1, 255, "123"];
	  //invalid parameters
	  var errInstanceidList = [ 12.234, -1, 256, "0x10"];
	  var startIndex = instanceidList.length;
	  
	  
	  //createNode and set instanceid
	  println( "begin to createNode and set instanceid" ) ;
	  var nodeHostName = db.listReplicaGroups().current().toObj().Group[0].HostName;
      for( var i = 0; i < instanceidList.length; i++)
      {         
         var nodeService = parseInt(RSRVPORTBEGIN) + i *10;         
         var nodePath = RSRVNODEDIR + nodeService; 
         var config = { instanceid : instanceidList[i]};
         rg.createNode(nodeHostName, nodeService, nodePath, config);
      }
	  
	  for( var i = 0; i < errInstanceidList.length; i++)
      {         
         var nodeService = parseInt(RSRVPORTBEGIN) + startIndex *10;         
         var nodePath = RSRVNODEDIR + nodeService; 
         var config = { instanceid : errInstanceidList[i]};
		 try
		 {
			 rg.createNode(nodeHostName, nodeService, nodePath, config);
			 throw "sessionAccess14069Error";
		 }catch( e )
		 {
			 if( -6 != e )
			 {
				 throw buildException("check set instanceid", e, "check the createNode and set instanceid",
                           -6, e);
			}
		}
		startIndex++;
      }
	  
	  println( "begin to start rg and check result" ) ;
      rg.start();
	  
	  // check result
	  checkResult(db, groupName, instanceidList);
	  //remove rg
	  println( "begin to remove rg" ) ;
	  db.removeRG(groupName);
	}
   catch( e )
   {
      throw e;
   }
   finally
   {
      if( db != null )
      {
         db.close()
      }
   }
}

function checkResult(db, groupName, instanceidList)
{
	var getDataGroupInfo = db.getRG(groupName).getDetail().current().toObj();
	for(var i = 0; i < instanceidList.length; i++ )
	{
		if(instanceidList[i] != 0 && instanceidList[i] != getDataGroupInfo.Group[i].instanceid)
		{
			throw buildException("check checkResult", null, "check the instanceid set result",
                           instanceidList[i], getDataGroupInfo.Group[i].instanceid);
		}
	}  
}

