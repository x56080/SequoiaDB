/* *****************************************************************************
@discretion: setSessionAttr(),set instatceid is same as the other node subscript
@author£º2018-1-24 wuyan  Init
***************************************************************************** */

main();

function main()
{	  
	try
	{	  
      var clName = CHANGEDPREFIX + "_sessionAcess14085";      
      var db = new Sdb(COORDHOSTNAME, COORDSVCNAME ) ; 
      
      //create group and node
      var groupName = "group14085";      
      var instanceidList = [ 2, 0, 0 ]; 
      createRGAndNode(db, groupName, instanceidList);  
         
      //create cl ,then insert data  
      var dbcl = commCreateCLByOption( db, COMMCSNAME, clName, {ReplSize:0,Group:groupName});  
      insertData( dbcl);
      
      //set one node instanceid is 2,the same as the nodesubscript is 2
      println("---begin to set instanceid ");       
      var instanceid = 2; 
      var accessCount = {}; 
      var expSvcNameList = getSvcNameList(db,groupName);           
      var expAccessNode = [expSvcNameList[0], expSvcNameList[ instanceid -1 ]] ;       
      for(  var i = 0; i < 20; i++ ) 
      {
         db.setSessionAttr( { PreferedInstance: instanceid } ) ; 
         var actAccessNode = getAccessNode( dbcl);
         checkAcessNodeResult( actAccessNode, expAccessNode );
         storageNodeAccessCount(actAccessNode, accessCount) ; 
      } 
      checkRandomAccessResult( expAccessNode, accessCount);         
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

