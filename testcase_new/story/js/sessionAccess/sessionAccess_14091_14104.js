/* *****************************************************************************
@discretion: 14091:setSessionAttr(),set [M/S/A],the preferedInstanceMode is random
                   setSessionAttr(),set [S/M/A],the preferedInstanceMode is random
             14104:set sessionAttr is S after insert data
@author£º2018-1-24 wuyan  Init
***************************************************************************** */

main();
function main()
{	  
	try
	{	  
	   var clName = CHANGEDPREFIX + "_sessionAcess14091";      
      var db = new Sdb(COORDHOSTNAME, COORDSVCNAME ) ; 
      
      //get groupname
      var groups = commGetGroups( db ) ;
      var groupName = groups[0][0]["GroupName"] ;   
         
      //create cl ,then insert data  
      var dbcl = commCreateCLByOption( db, COMMCSNAME, clName, {ReplSize:0,Group:groupName});  
      insertData( dbcl);
      
      println("---begin to test testcase14091"); 
      //set [M/S/A] ,set the preferedInstanceMode is ordered 
      var queryInstanceidList = ["M","S","A"];            
      db.setSessionAttr( { PreferedInstance: queryInstanceidList,PreferedInstanceMode: "random"  } ) ; 
      var actAccessNode = getAccessNode( dbcl);      
      checkAccessNodeIsPrimary( actAccessNode, groupName, true );           
      
      //set [S/M/A] ,set the preferedInstanceMode is ordered(test 14104)
      println("---begin to set the [S/M/A]"); 
      var queryInstanceidList1 = ["S","M","A"];            
      db.setSessionAttr( { PreferedInstance: queryInstanceidList1,PreferedInstanceMode: "random"  } ) ; 
      var actAccessNode = getAccessNode( dbcl); 
      checkAccessNodeIsPrimary( actAccessNode, groupName, false ); 
      println("---end to test testcase14091 ");  
      
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



