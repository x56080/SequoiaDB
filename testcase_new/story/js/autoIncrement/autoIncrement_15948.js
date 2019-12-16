/***************************************************************************
@Description : seqDB-15948:range分区表，shardKey同时为自增字段，不指定自增字段插入记录
@Modify list :
              2018-10-17  zhaoyu  Create
****************************************************************************/
function main ()
{
   var dataGroupNames = getDataGroupNames();
   if( commIsStandalone( db ) || dataGroupNames.length < 2 )
   {
      println( "Deploy is standalone or only one group" );
      return;
   }

   var clName = COMMCLNAME + "_15948";
   var field = "id";
   commDropCL( db, COMMCSNAME, clName, true, true );

   var cacheSize = 10;
   var acquireSize = 1;
   var dbcl = commCreateCL( db, COMMCSNAME, clName, {
      ShardingType: "range", ShardingKey: { id: 1 }, Group: dataGroupNames[0],
      AutoIncrement: { Field: field, CacheSize: cacheSize, AcquireSize: acquireSize }
   } );

   var clID = getCLID( COMMCSNAME, clName );
   var sequenceName = "SYS_" + clID + "_" + field + "_SEQ";
   var expIncrementArr = [{ Field: "id", SequenceName: sequenceName }];
   checkAutoIncrementonCL( COMMCSNAME, clName, expIncrementArr );

   var expSequenceObj = { AcquireSize: acquireSize, CacheSize: cacheSize };
   checkSequence( sequenceName, expSequenceObj );

   dbcl.split( dataGroupNames[0], dataGroupNames[1], { id: 50 } );

   var doc = [];
   var expR = [];
   for( var i = 0; i < 100; i++ )
   {
      doc.push( { a: i, b: i } );
      expR.push( { a: i, b: i, id: 1 + i } );
   }
   dbcl.insert( doc );

   checkCountFromNode( dataGroupNames[0], COMMCSNAME, clName, 49 );
   checkCountFromNode( dataGroupNames[1], COMMCSNAME, clName, 51 );
   var actR = dbcl.find().sort( { a: 1 } );
   checkRec( actR, expR );

   commDropCL( db, COMMCSNAME, clName, true, true );
}
try
{
   main();
}
catch( e )
{
   if( e.constructor === Error )
   {
      println( e.stack );
   }
   throw e;
}
