/* *****************************************************************************
@discretion: setSessionAttr(),set instatceid and timeout, query timeout
             metaopr timeout:test createcl timeout
             crud timeout :test insert/query timeout
@author£º2018-1-29 wuyan  Init
***************************************************************************** */

main();
function main()
{	  
	try
	{      
      var sdb = new Sdb(COORDHOSTNAME, COORDSVCNAME ) ;
      if( true == commIsStandalone( sdb ) )
      {
         println( "run mode is standalone" );
         return;
      } 
      
      //create group and node  
      var groupName = "group14114";   
      var instanceidList = [ 14, 29, 15];      
      createRGAndNode(sdb, groupName, instanceidList);      
      
      //create cl and insert data
      var csName = CHANGEDPREFIX + "_cs14114";
      var clName = CHANGEDPREFIX + "_sessionAcess14114";  
      commCreateCS( sdb, csName, false, "Failed to create CS.");
      var options = {ReplSize:0,Group:groupName};
      var dbcl = commCreateCLByOption( sdb, csName, clName, options, true, true );       
      insertData( dbcl);
              
      println("---begin to test query timeout ");   
      testQueryTimeout(csName, clName);
      
      println("---begin to test createcl timeout ");  
      setCreateCLTimeout(csName); 
      
      println("---begin to test insertcl timeout ");
      setInsertTimeout(csName, clName);      
      
      println("---begin dropcs ");      
      commDropCS( sdb, csName, false, "Failed to drop CS.");       
      
      println("---begin to remove RG ");        
      sdb.removeRG(groupName);
   }
   catch( e )
   {
      throw buildException( "test session14114", e );    
   }
   finally
   {
      if( db != null )
      {
         sdb.close()
      }
   }
}

function testQueryTimeout(csName, clName)
{   
   try
   {
      var sdb = new Sdb(COORDHOSTNAME, COORDSVCNAME );
      var instanceid = 15;
      sdb.setSessionAttr( { PreferedInstance: instanceid, Timeout : 1} );      
      var dbcl = sdb.getCS(csName).getCL(clName);
      var rc = dbcl.find().sort({a:1});
      while( rc.next() )
      {    
         var atcObj = rc.current().toObj();      
      } 
      throw "need throw error";
   }
   catch( e )
   {
      if( e !== -13 )
            throw buildException( "check query timeout", e );     
   }
   finally
   {
      sdb.setSessionAttr( { Timeout : -1} );
      if ( rc != null)
      {         
         rc.close();
      }
      if( sdb != null)
      {
         sdb.close();
      }
      
   }
}

function setCreateCLTimeout(csName)
{   
   try
   {
      var db1 = new Sdb(COORDHOSTNAME, COORDSVCNAME );
      
      db1.setSessionAttr( { Timeout : 1} );
      var clName = "test14114";
      var options = {ReplSize:0,ShardingKey:{a:1}};
      db1.getCS(csName).createCL(clName, options);
      throw "need throw error";
   }
   catch( e )
   {
      if( e !== -13 )
            throw buildException( "check createcl timeout", e );     
   }
   finally
   {
      db1.setSessionAttr( { Timeout : -1} );
      if ( db1 != null)
      {         
         db1.close();
      }
      
   }
}

function setInsertTimeout(csName, clName)
{   
   try
   {
      var db = new Sdb(COORDHOSTNAME, COORDSVCNAME );
      var dbcl = db.getCS(csName).getCL(clName);
      db.setSessionAttr( { Timeout : 1} );
      var docs = [];
      for( var i = 0; i < number; ++i )
      {      
         var no = i;
         var a = i;
         var user = "test"+i;
         var phone = 13700000000+i;
         var time = new Date().getTime(); 
         var doc = {no:no, a:a,customerName:user, phone:phone, openDate:time};      
         //data example: {"no":5, customerName:"test5", "phone":13700000005, "openDate":1402990912105
         
         docs.push( doc );
      }	
      dbcl.insert( docs );       
      throw "need throw error";
   }
   catch( e )
   {
      if( e !== -13 )
            throw buildException( "check timeout", e );     
   }
   finally
   {
      db.setSessionAttr( { Timeout : -1} );
      if ( db != null)
      {         
         db.close();
      }
      
   }
}

