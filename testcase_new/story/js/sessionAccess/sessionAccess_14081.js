/* *****************************************************************************
@discretion: setSessionAttr(),set a instatceid
@author£º2018-1-22 wuyan  Init
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
      var groupName = "group14081";      
      var instanceidList = [ 0, 0, 15];      
      createRGAndNode(db, groupName, instanceidList); 
      
      //create cl and insert data
      var clName = CHANGEDPREFIX + "_sessionAcess14081";  
      var options = {ReplSize:0,Group:groupName};
      var dbcl = commCreateCLByOption( db, COMMCSNAME, clName, options, true, true );       
      insertData( dbcl);
      
      //set session instanceid 
      var instanceid = 15;
      db.setSessionAttr( { PreferedInstance: instanceid } )      
      
      //check the query node 
      var queryNode = getAccessNode(dbcl);
      println("queryNode="+queryNode);
      var expSvcNameList = getSvcNameList(db,groupName); 
      var expNodeName = expSvcNameList[2];
      checkAcessNodeResult( queryNode, expNodeName );       
      
      //get session and check result
      var expSessionInfo = {PreferedInstance:15, PreferedInstanceMode:"random", "Timeout": -1}; 
      getSessionAndCheckResult(db, expSessionInfo);
      
      commDropCL( db, COMMCSNAME, clName, true, true,
               "clear collection in the beginning" ) ;
      db.removeRG(groupName);
   }
   catch( e )
   {
      throw e ;
   }
   finally
   {
      if( db != null )
      {
         db.close()
      }
   }
}

