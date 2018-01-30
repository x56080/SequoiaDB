/* *****************************************************************************
@discretion: setSessionAttr(),set non-existent instatceid
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
        
      //get group and node 
      var groups = commGetGroups( db ) ;
      var clGroupName = groups[0][0]["GroupName"] ; 
       
      var clName = CHANGEDPREFIX + "_sessionAcess14083"; 
      var dbcl = commCreateCLByOption( db, COMMCSNAME, clName, {ReplSize:0,Group:clGroupName}, true, true );  
      insertData( dbcl);
      
      println("---begin to set multiple non-existen instanceid ");
      //set multiple non-existent instatceid
      var instanceid = [39,12];
      var accessCount = {};
      var expSvcName = getSvcNameList(db,clGroupName);
      for( var i = 0; i < 20; i++ )
      {         
         db.setSessionAttr( { PreferedInstance: instanceid } )                
         var actSvcName = getAccessNode(dbcl);
         storageNodeAccessCount(actSvcName, accessCount) ;          
      }
      checkRandomAccessResult( expSvcName, accessCount);
      
      var expSessionInfo = {PreferedInstance:instanceid, PreferedInstanceMode:"random", "Timeout": -1}; 
      getSessionAndCheckResult(db, expSessionInfo);
      println("---end to set multiple non-existen instanceid ");
      
      //set a non-existent instatceid
      println("---begin to set a non-existen instanceid ");
      var instanceid1 = 39;      
      var expNodeName1 = expSvcName[(instanceid1-1)%expSvcName.length];
      for( i = 0; i < 10; i++ )
      {
         db.setSessionAttr( { PreferedInstance: instanceid1 } )
         var queryNode = getAccessNode(dbcl);     
         checkAcessNodeResult( queryNode, expNodeName1 );                
      }
      var expSessionInfo1 = {PreferedInstance:instanceid1, PreferedInstanceMode:"random", "Timeout": -1}; 
      getSessionAndCheckResult(db, expSessionInfo1);
      println("---end to set a non-existen instanceid ");
      
      commDropCL( db, COMMCSNAME, clName, true, true,
               "clear collection in the beginning" ) ;
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



