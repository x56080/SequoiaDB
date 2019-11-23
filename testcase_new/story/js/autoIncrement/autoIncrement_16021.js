/******************************************************************************
@Description :   seqDB-16021:  修改当前值 
@Modify list :   2018-10-23    xiaoni Zhao  Init
******************************************************************************/
function main()
{
   if(commIsStandalone( db ))
   {
      println("Deploy is standalone");
      return;
   }  
   
   var clName = COMMCLNAME + "_16021";
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
   checkRec( rc, expRecs.sort(compare("id1")) );
   
   //alter attributes and check
   dbcl.setAttributes({ AutoIncrement : { Field : "id1", CurrentValue : 10 } });
   
   var clID = getCLID(COMMCSNAME, clName);
   var sequenceName = "SYS_" + clID + "_id1_SEQ";
   var cursor = db.snapshot(SDB_SNAP_SEQUENCES, { Name : sequenceName });
   var currentValue = cursor.current().toObj().CurrentValue;
   if( currentValue !== 10)
   {
      throw new Error("expect is 10 but currentValue is " + currentValue);
   }
   
   //insert records and check
   for( var i = 0; i < coordNodes.length; i++ )
   {
      var coord = new Sdb( coordNodes[ i ] );
      var cl = coord.getCS( COMMCSNAME ).getCL( clName );
      cl.insert( { "a" : i, "b" : i } );
      expRecs.push({ "a" : i, "b" : i, "id1" : i*acquireSize + 11 });
      coord.close();
   }
    
   var rc = dbcl.find().sort( { "id1" : 1 } );
   checkRec( rc, expRecs.sort(compare("id1")) );
   
   //alter attributes and check
   dbcl.setAttributes({ AutoIncrement : { Field : "id1", CurrentValue : 4000 } });
   
   var clID = getCLID(COMMCSNAME, clName);
   var sequenceName = "SYS_" + clID + "_id1_SEQ";
   var cursor = db.snapshot(SDB_SNAP_SEQUENCES, { Name : sequenceName });
   var currentValue = cursor.current().toObj().CurrentValue;
   if( currentValue !== 4000)
   {
      throw new Error( "Expect is 4000 but currentValue is " + currentValue );
   }
   
   //insert records and check
   var coordNodes = getCoordNodeNames();
   for( var i = 0; i < coordNodes.length; i++ )
   {
      var coord = new Sdb( coordNodes[ i ] );
      var cl = coord.getCS( COMMCSNAME ).getCL( clName );
      cl.insert( { "a" : i, "b" : i } );
      expRecs.push({ "a" : i, "b" : i, "id1" : i*acquireSize + 4001 });
      coord.close();
   }
    
   var rc = dbcl.find().sort( { "id1" : 1 } );
   checkRec( rc, expRecs.sort(compare("id1")) );
   
   //alter attributes
   dbcl.setAttributes({ AutoIncrement : { Field : "id1", CurrentValue : { "$numberLong" : "9223372036854775807" } } });

   //insert records and check
   try
   {
      dbcl.insert( { "q" : 2 } );
      throw new Error( "insert ERROR" );
   }catch(e)
   {
       if(e !== -325)
       {
           throw new Error(e);
       }
   }
   var rc = dbcl.find().sort( { "id1" : 1 } );
   checkRec( rc, expRecs.sort(compare("id1")) );
   
   //alter attributes
   dbcl.setAttributes({ AutoIncrement : { Field : "id1", CurrentValue : 4 } });
   
   //insert records and check
   var coordNodes = getCoordNodeNames();
   for( var i = 0; i < coordNodes.length; i++ )
   {
      var coord = new Sdb( coordNodes[ i ] );
      var cl = coord.getCS( COMMCSNAME ).getCL( clName );
      cl.insert( { "a" : i, "b" : i } );
      expRecs.push({ "a" : i, "b" : i, "id1" : i*acquireSize + 5 });
      coord.close();
   }
    
   var rc = dbcl.find().sort( { "id1" : 1 } );
   checkRec( rc, expRecs.sort(compare("id1")) );
   
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
