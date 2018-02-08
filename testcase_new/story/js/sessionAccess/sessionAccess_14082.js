/* *****************************************************************************
@discretion: setAttr() is M/S/A,the session is specified in the configuration(A is default and testover)
@author£º2018-1-24 wuyan  Init
***************************************************************************** */

main();
function main()
{	  
	try
	{	 
	   var db = new Sdb(COORDHOSTNAME, COORDSVCNAME ) ;  
	   if( true == commIsStandalone( db ) )
      {
         println( "run mode is standalone" );
         return;
      }      
           
      //create group and node
      var groupName = "group14082";       
      createRGAndNode(db, groupName);
     
      //create coord node 
      var nodeHostName = db.listReplicaGroups().current().toObj().Group[0].HostName; 
      var nodeService = parseInt(RSRVPORTBEGIN) + 200;
      var nodeService1 = parseInt(RSRVPORTBEGIN) + 210;
      var coordRg = db.getRG("SYSCoord");
      coordRg.createNode(nodeHostName, nodeService, RSRVNODEDIR + nodeService, {preferedinstance:"M"}); 
      coordRg.createNode(nodeHostName, nodeService1, RSRVNODEDIR + nodeService1, {preferedinstance:"S"}); 
      coordRg.start();
      
      //create cl ,then insert data 
      var clName = CHANGEDPREFIX + "_sessionAcess14082";        
      var dbcl = commCreateCLByOption( db, COMMCSNAME, clName, {ReplSize:0,Group:groupName});  
      insertData( dbcl);        
      
      //test sessionAttr is M on coord
      var coordUrl = new Sdb(nodeHostName, nodeService ) ;   
      var queryInstanceid  = "M"; 
      testsessionAccess( coordUrl, clName, groupName, queryInstanceid, true);
      
      //test sessionAttr is S on coord
      var coordUrl1 = new Sdb(nodeHostName, nodeService1 ) ;   
      var queryInstanceid1  = "S"; 
      testsessionAccess( coordUrl1, clName, groupName, queryInstanceid1, false);
      
      commDropCL( db, COMMCSNAME, clName, true, true,
               "clear collection in the beginning" ) ;      
      coordRg.removeNode(nodeHostName, nodeService);
      coordRg.removeNode(nodeHostName, nodeService1);
      db.removeRG(groupName);
   }
   catch( e )
   {
      throw e;
   }
   finally
   {
      if( coordUrl != null )
      {
         coordUrl.close()
      }
      if( coordUrl1 != null )
      {
         coordUrl1.close()
      }
   }   
}

function testsessionAccess( coordUrl, clName, groupName, queryInstanceid, isPrimary)
{
   try
	{
	   println("---begin to test query "+queryInstanceid);
	   //test setsession       
      var querydb = coordUrl.getCS(COMMCSNAME).getCL(clName) ;        
      var actAccessNode = getAccessNode( querydb);         
      checkAccessNodeIsPrimary( actAccessNode, groupName, isPrimary);
      
      //test getsession
      var expSessionInfo = {PreferedInstance:queryInstanceid, PreferedInstanceMode:"random", "Timeout": -1}; 
      getSessionAndCheckResult(coordUrl, expSessionInfo);              
      println("---end to test query "+queryInstanceid);      
   }
   catch( e )
   {
      throw buildException( "testSessionAccess14082 fail", e);     
   }      
}

