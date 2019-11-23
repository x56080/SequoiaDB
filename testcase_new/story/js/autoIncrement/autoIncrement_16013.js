/******************************************************************************
@Description :   seqDB-16013:   修改递增值 
@Modify list :   2018-10-22    xiaoni Zhao  Init
******************************************************************************/
function main()
{
   if(commIsStandalone( db ))
   {
      println("Deploy is standalone");
      return;
   }
    
   var clName = COMMCLNAME + "_16013";
   var increment = 2;
   var acquireSize = 10;
   
   commDropCL( db, COMMCSNAME, clName );
   
   var dbcl = commCreateCLByOption( db, COMMCSNAME, clName, { AutoIncrement : { Field : "id1", AcquireSize : acquireSize } } );
   
   //insert records and check
   var coordNodes = getCoordNodeNames();
   var expRecs = [];
   for( var i = 0; i < coordNodes.length; i++ )
   {
      var coord = new Sdb( coordNodes[ i ] );
      var cl = coord.getCS( COMMCSNAME ).getCL( clName );
      cl.insert( { "a" : i, "b" : i } );
      expRecs.push({ "a" : i, "b" : i, "id1" : 1 + i*acquireSize});
      coord.close();
   }
    
   var rc = dbcl.find().sort( { "id1" : 1 } );
   checkRec( rc, expRecs );
   
   //alter attributes and check
   dbcl.setAttributes({ AutoIncrement : { Field : "id1", Increment : increment } });
   
   var clID = getCLID(COMMCSNAME, clName);
   var sequenceName = "SYS_" + clID + "_id1_SEQ";
   var cursor = db.snapshot(SDB_SNAP_SEQUENCES, { Name : sequenceName });
   if( cursor.current().toObj().Increment !== increment)
   {
      throw new Error("alter failed!");
   }
   
   //insert records and check
   for( var i = 0; i < coordNodes.length; i++ )
   {
      var coord = new Sdb( coordNodes[ i ] );
      var cl = coord.getCS( COMMCSNAME ).getCL( clName );
      cl.insert( { "a" : i, "b" : i } );
      expRecs.push({ "a" : i, "b" : i, "id1" : 1 + coordNodes.length*acquireSize + 1 + i*increment*acquireSize});
      coord.close();
   }
    
   var rc = dbcl.find().sort( { "id1" : 1 } );
   checkRec( rc, expRecs );
   
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
