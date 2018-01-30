/* *****************************************************************************
@discretion: setSessionAttr(),set instanceid and [M/S/A],the instanceid does not exist
             test the following scenes:
             a: set multiple instanceid and ["M"]/["m"]
             b: set multiple instanceid and ["S"]/["s"]
             c: set multiple instanceid and ["A"]/["a"]
             d: set multiple instanceid and [M/S/A]
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
      var groupName = "group14094";      
      var instanceidList = [ 30, 124, 8, 22 ]; 
      var nodeNum = 4;
      createRGAndNode(db, groupName, instanceidList, nodeNum);      
         
      //create cl ,then insert data  
      var clName = CHANGEDPREFIX + "_sessionAcess14094"; 
      var dbcl = commCreateCLByOption( db, COMMCSNAME, clName, {ReplSize:0,Group:groupName});  
      insertData( dbcl);
      
      //test a: set multiple instanceid and ["M"]/["m"]
      setSessionIsInstanceAndMm(db, dbcl, groupName); 
      
      //test b: set multiple instanceid and ["S"]
      setSessionIsInstanceAndSs(db, dbcl, groupName);
      
      //test c: set multiple instanceid and ["A"]
      
      setSessionIsInstanceAndAa(db, dbcl, groupName);      
      
      //test d: set multiple instanceid and ["M/S/A"]
      setSessionIsInstanceAndMSA(db, dbcl, groupName);       
      
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

function setSessionIsInstanceAndMm(db, dbcl, groupName)
{
   println("---begin to test set multiple instanceid and ['M']/['m'] ");  
   var queryInstanceidList = [1, 224, 38, "M"]; 
   setSessionAttrAndCheckResult(db, dbcl, queryInstanceidList,groupName, true) ;  
   //a: set multiple instanceid and ["m"] 
   var queryInstanceidList_a2 = [1, 224, 38, "m"];      
   setSessionAttrAndCheckResult(db, dbcl, queryInstanceidList_a2, groupName,true) ;     
   println("---end to test set multiple instanceid and ['M']/['m'] ");  
}

function setSessionIsInstanceAndSs(db, dbcl, groupName)
{
   println("---begin to test set multiple instanceid and ['S']/['s'] ");  
   var queryInstanceidList_b1 = [1, 224, 38, "S"]; 
   setSessionAttrAndCheckResult(db, dbcl, queryInstanceidList_b1,groupName, false) ; 
   //b: set multiple instanceid and ["s"]
   var queryInstanceidList_b2 = [1, 224, 38,"s"];       
   setSessionAttrAndCheckResult(db, dbcl, queryInstanceidList_b2, groupName,false) ; 
   println("---end to test set multiple instanceid and ['S']/['s'] ");   
}

function setSessionIsInstanceAndAa(db, dbcl, groupName)
{
   println("---begin to test set multiple instanceid and ['A']/['a'] ");
   var expSvcNameList = getSvcNameList(db,groupName);   
   var queryInstanceidList_c1 = [1, 224, 38, "A"];   
   db.setSessionAttr( { PreferedInstance: queryInstanceidList_c1} ) ;         
   var queryNode_c1 = getAccessNode(dbcl);      
   checkAcessNodeResult( queryNode_c1, expSvcNameList );
   //c: set multiple instanceid and ["a"],the instanceid:30 is masternode
   var queryInstanceidList_c2 = [1, 224, 38,"a"];
   db.setSessionAttr( { PreferedInstance: queryInstanceidList_c2} ) ;         
   var queryNode_c2 = getAccessNode(dbcl);      
   checkAcessNodeResult( queryNode_c2, expSvcNameList ); 
   println("---end to test set multiple instanceid and ['A']/['a'] ");  
}

function setSessionIsInstanceAndMSA(db, dbcl, groupName)
{
   println("---begin to test set multiple instanceid and ['M/S/A'] ");  
   var queryInstanceidList_d = [1, 224, 38, "M", "S", "A"];      
   setSessionAttrAndCheckResult(db, dbcl, queryInstanceidList_d, groupName, true) ;      
   println("---end to test set multiple instanceid and ['M/S/A'] ");  
}

function setSessionAttrAndCheckResult(db, dbcl, queryInstanceidList, groupName,isPrimary)
{
   db.setSessionAttr( { PreferedInstance: queryInstanceidList} ) ;   
   var queryNode = getAccessNode( dbcl);
   checkAccessNodeIsPrimary( queryNode, groupName, isPrimary );   
}