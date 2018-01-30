/* *****************************************************************************
@discretion: setSessionAttr(),set instatceid is node subscript
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
      var groupName = "group14084"; 
      createRGAndNode(db, groupName);
      var expSvcNameList = getSvcNameList(db,groupName);
        
      //create cl and insert data
      var clName = CHANGEDPREFIX + "_sessionAcess14084";   
      var dbcl = commCreateCLByOption( db, COMMCSNAME, clName, {ReplSize:0,Group:groupName}, true, true );  
      insertData( dbcl);
      
      //set instanceid 
      println("---begin to set instanceid ");       
      var instanceid = Math.floor(Math.random()*expSvcNameList.length + 1 );      
      db.setSessionAttr( { PreferedInstance: instanceid } ) 
      
      //check the query node     
      var queryNode = getAccessNode(dbcl);
      var expNodeName = expSvcNameList[ instanceid -1 ];        
      checkAcessNodeResult( queryNode, expNodeName );    
      println("---end to set instanceid ");        
      
      commDropCL( db, COMMCSNAME, clName, true, true,
               "clear collection in the beginning" ) ;
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

