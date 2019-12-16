/******************************************************************************
@Description : 1. Test split and insert thousands data.[query/update/remove]
@Modify list :
               2014-6-25  xiaojun Hu  Init
******************************************************************************/

function main ( db )
{
   //@ clean before
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true,
      "clear collection space in the beginning" );
   // Inspect and Get groups
   var group = new Array();
   group = getGroup( db );
   if( group.length < 2 )
   {
      println( "Don't have enough group, count = " + group.length );
      throw "NotEnoughGroup";
   }
   // Create CS and CL
   commCreateCL( db, COMMCSNAME, COMMCLNAME, {
      ShardingKey: { "No": -1 },
      ShardingType: "hash", Partition: 1024,
      ReplSize: 0
   },
      true, false, "create collection in domain" );
   // Get Group the COMMCSNAME located in and Split the CS to other group
   // [Testing Point ]
   getTwoGroupSplit( db, COMMCSNAME, COMMCLNAME, { Partition: 512 },
      { Partition: 1024 } );
   // Insert Data
   insertData( db, COMMCSNAME, COMMCLNAME, 1000 );
   println( "Success to insert data, count = 1000" );
   // Query Data
   queryData( db, COMMCSNAME, COMMCLNAME );
   // Update Data
   updateData( db, COMMCSNAME, COMMCLNAME );
   // Remove Data
   removeData( db, COMMCSNAME, COMMCLNAME );
   //@ clean end
   commDropCL( db, COMMCSNAME, COMMCLNAME, false, false,
      "clear collection space in the end, correct" );
}

try
{
   if( true == commIsStandalone( db ) )
      throw "ModeStandAlone";
   main( db );
   db.close();
}
catch( e )
{
   if( "NotEnoughGroup" == e )
      println( "Don't have enough group" );
   else if( "ModeStandAlone" == e )
      println( "This is standalone mode" );
   else
   {
      commDropCL( db, COMMCSNAME, COMMCLNAME, false, false,
         "clear collection space in the end, correct" );
      db.close();
      throw e;
   }
}
