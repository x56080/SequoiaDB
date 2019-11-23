/******************************************************************************
@Description :   seqDB-16019:  修改Cycled属性值   
@Modify list :   2018-10-22    xiaoni Zhao  Init
******************************************************************************/
function main()
{
   if(commIsStandalone( db ))
   {
      println("Deploy is standalone");
      return;
   }  
   
   var clName = COMMCLNAME + "_16019";
   
   commDropCL( db, COMMCSNAME, clName );
   
   var dbcl = commCreateCLByOption( db, COMMCSNAME, clName, { AutoIncrement : { Field : "id1", MaxValue : 3000 } } );
   
   //insert records and check
   var coordNodes = getCoordNodeNames();
   if(coordNodes.length !== 3)
   {
      return ;
   }
   var expRecs = [];
   for( var i = 0; i < coordNodes.length; i++ )
   {
      var coord = new Sdb( coordNodes[ i ] );
      var cl = coord.getCS( COMMCSNAME ).getCL( clName );
      cl.insert( { "a" : i, "b" : i } );
      expRecs.push({ "a" : i, "b" : i, "id1" : 1 + i*1000});
      coord.close();
   }
    
   var rc = dbcl.find().sort( { "id1" : 1 } );
   checkRec( rc, expRecs );
   
   //alter attributes default and check
   dbcl.setAttributes({ AutoIncrement : { Field : "id1", Cycled : false } });
   
   var clID = getCLID(COMMCSNAME, clName);
   var sequenceName = "SYS_" + clID + "_id1_SEQ";
   var cursor = db.snapshot(SDB_SNAP_SEQUENCES, { Name : sequenceName });
   if( cursor.current().toObj().Cycled !== false)
   {
      throw new Error("alter failed!");
   }
 
   //insert records and check
   for( var i = 0; i < coordNodes.length; i++ )
   {
      var coord = new Sdb( coordNodes[ i ] );
      var cl = coord.getCS( COMMCSNAME ).getCL( clName );
      try
      {
         cl.insert( { "a" : i, "b" : i } );
         throw "insert error!";
      }catch(e)
      {
         if(e !== -325)
         {
            throw new Error(e);
         }
      }
      coord.close();
   }
    
   var rc = dbcl.find();
   checkRec( rc, expRecs );
   println("===check Cycled succeed!===");
   
   //alter Cycled true
   dbcl.setAttributes({ AutoIncrement : { Field : "id1", Cycled : true } });
   
   var cursor = db.snapshot(SDB_SNAP_SEQUENCES, { Name : sequenceName });
   if( cursor.current().toObj().Cycled !== true)
   {
      throw new Error("alter failed!");
   }
   
   //insert records and check
   for( var i = 0; i < coordNodes.length; i++ )
   {
      var coord = new Sdb(coordNodes[i]);
      var cl = coord.getCS(COMMCSNAME).getCL(clName);
      cl.insert( { "a" : i, "b" : i } );
      expRecs.push({ "a" : i, "b" : i, "id1" : 1 + i*1000 });
      coord.close();
   }
   
   var rc = dbcl.find();
   checkRec( rc, expRecs );
   println("===check Cycled true succeed!===");
 
   //alter Cycled false
   dbcl.setAttributes({ AutoIncrement : { Field : "id1", Cycled : false } });
   
   var cursor = db.snapshot(SDB_SNAP_SEQUENCES, { Name : sequenceName });
   if( cursor.current().toObj().Cycled !== false )
   {
      throw new Error("alter failed!");
   }
   
   //insert records and check
   for( var i = 0; i < coordNodes.length; i++ )
   {
      var coord = new Sdb(coordNodes[i]);
      var cl = coord.getCS(COMMCSNAME).getCL(clName);
      try
      {
         cl.insert( { "a" : i, "b" : i } );
         throw "insert error";
      }catch(e)
      {
         if(e !== -325)
         {
            throw new Error(e);
         }
      }
      coord.close();
   }
      
   var rc = dbcl.find().sort({ "id1" : 1 });
   checkRec( rc, expRecs.sort(compare("id1")));
   println("===check Cycled false succeed!===");   

   commDropCL( db, COMMCSNAME, clName );
}
try
{
   main();
}
catch(e)
{
   if ( e.constructor === Error )
   {
      println(e.stack) ;  
   }
   throw e ;
}
