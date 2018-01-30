/* *****************************************************************************
@discretion: setSessionAttr(),set instanceid is 8/9/10( old version 8 is master,9 is slave,
               new version query node by instanceid)
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
      var groupName = "group14096";      
      var instanceidList = [ 9, 8, 10, 11 ]; 
      var nodeNum = 4;
      createRGAndNode(db, groupName, instanceidList, nodeNum);
      var expSvcNameList = getSvcNameList(db,groupName);        
      
      //create cl ,then insert data 
      var clName = CHANGEDPREFIX + "_sessionAcess14096";      
      var dbcl = commCreateCLByOption( db, COMMCSNAME, clName, {ReplSize:0,Group:groupName});  
      insertData( dbcl);
      
      //set instanceid is 8,the query node is slave node
      var queryInstanceid = 8; 
      setSessionAttrAndCheckResult(db, dbcl, groupName, queryInstanceid, expSvcNameList[1], false );
      
      //set instanceid is 9,the query node is master node
      var queryInstanceid1 = 9; 
      setSessionAttrAndCheckResult(db, dbcl, groupName, queryInstanceid1, expSvcNameList[0], true );
      
      //set instanceid is 10,the query node is slave node
      var queryInstanceid2 = 10;      
      setSessionAttrAndCheckResult(db, dbcl, groupName,queryInstanceid2, expSvcNameList[2], false );        
      
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

function setSessionAttrAndCheckResult(db,dbcl, groupName,queryInstanceid, expQueryNode, isPrimary )
{
   println("---begin to set and query instanceid is "+queryInstanceid);
   db.setSessionAttr( { PreferedInstance: queryInstanceid } ) ;  
   var queryNode = getAccessNode( dbcl);
   checkAcessNodeResult( queryNode, expQueryNode );
   checkAccessNodeIsPrimary( queryNode, groupName, isPrimary );
   println("---end to set and query instanceid is "+queryInstanceid);
}