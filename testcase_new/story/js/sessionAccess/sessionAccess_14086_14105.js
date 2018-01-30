/* *****************************************************************************
@discretion: 14086:setSessionAttr(),set multiple instatceids,
                           the preferedInstanceMode is random
             14105:the session is specified in the configuration,respecified on the coord
@author£º2018-1-24 wuyan  Init
***************************************************************************** */

main();
function main()
{	  
	try
	{	  
	   if( true == commIsStandalone( db ) )
      {
         println( "run mode is standalone" );
         return;
      } 
      
      var clName = CHANGEDPREFIX + "_sessionAcess14086";      
      //create group and node
      var groupName = "group14086";      
      var instanceidList = [ 10, 4, 25, 255 ]; 
      var nodeNum = 4;
      createRGAndNode(db, groupName, instanceidList, nodeNum);
      var expSvcNameList = getSvcNameList(db,groupName);     
      
      //create coord node 
      var nodeHostName = db.listReplicaGroups().current().toObj().Group[0].HostName; 
      var nodeService = parseInt(RSRVPORTBEGIN) + 100;
      var coordRg = db.getRG("SYSCoord");
      coordRg.createNode(nodeHostName, nodeService, RSRVNODEDIR + nodeService, {preferedinstance:"4,255",preferedinstanceMode:"random" }); 
      coordRg.start();
      
      //create cl ,then insert data       
      var dbcl = commCreateCLByOption( db, COMMCSNAME, clName, {ReplSize:0,Group:groupName});  
      insertData( dbcl);     
       
      //qurey node and check the access node
      testsessionAccess14086(nodeHostName, nodeService, clName,expSvcNameList);
      
      //test testcase14110:reset sessionAttr on coord
      var coordUrl = new Sdb(nodeHostName, nodeService ) ;
      testsessionAccess14105(coordUrl, clName, expSvcNameList);
      
      commDropCL( db, COMMCSNAME, clName, true, true,
               "clear collection in the beginning" ) ;
      db.removeRG(groupName);
      coordRg.removeNode(nodeHostName, nodeService);
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
   }   
}

function createCoordNode(db)
{
   try
   {
      var rg = db.getRG("SYSCoord");
      var nodeHostName = db.listReplicaGroups().current().toObj().Group[0].HostName;
      var nodeService = parseInt(RSRVPORTBEGIN) + 100;         
      var nodePath = RSRVNODEDIR + nodeService;          
      var config = {preferedinstance:"1,2,3",preferedinstanceMode:"random" };         
      rg.createNode(nodeHostName, nodeService, nodePath, config);
   }
   catch( e )
   {
      throw e;
   }
   return rg;
    
}

function testsessionAccess14086(nodeHostName, nodeService,clName, expSvcNameList)
{
   try
	{
	   println("---begin to test testcase14086 ");          
      var accessCount = {};             
      var expAccessNode = [expSvcNameList[1], expSvcNameList[3]] ;           
      for(  var i = 0; i < 20; i++ ) 
      {      
         var coordUrl = new Sdb(nodeHostName, nodeService ) ;
         var querydb = coordUrl.getCS(COMMCSNAME).getCL(clName) ;        
         var actAccessNode = getAccessNode( querydb);         
         checkAcessNodeResult( actAccessNode, expAccessNode );
         storageNodeAccessCount(actAccessNode, accessCount) ; 
      } 
      checkRandomAccessResult( expAccessNode, accessCount);         
      println("---end to test testcase14086 ");       
   }
   catch( e )
   {
      throw buildException( "testSessionAccess14086 fail", e);     
   }      
}

function testsessionAccess14105(coordUrl, clName, expSvcNameList)
{
   try
	{
	   println("---begin to test testcase14105 "); 
	   var queryInstanceid = 25; 	     
	   coordUrl.setSessionAttr( { PreferedInstance: queryInstanceid} ) ;  
	   
	   var querydb = coordUrl.getCS(COMMCSNAME).getCL(clName) ;     
      var queryNode = getAccessNode( querydb);  
      checkAcessNodeResult( queryNode, expSvcNameList[2] ) ; 
      println("---end to test testcase14105 ");       
   }
   catch( e )
   {
      throw buildException( "testSessionAccess14105 fail", e);     
   }      
}