/***************************************************************************
@Description : seqDB-15944:集合中存在多自增字段，插入记录
@Modify list :
              2018-10-16  zhaoyu  Create
****************************************************************************/
var sortField = 0;
function main ()
{
   if( commIsStandalone( db ) )
   {
      println( "Deploy is standalone" );
      return;
   }

   var clName = COMMCLNAME + "_15944";
   commDropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName, {
      AutoIncrement: [{ Field: "id1", AcquireSize: 10 },
      { Field: "id2", AcquireSize: 1, Increment: -1, CacheSize: 10, AcquireSize: 1 }]
   } );
   var expR = [];
   for( var j = 0; j < 100; j++ )
   {
      var doc = [];
      for( var i = 1; i < 6; i++ )
      {
         doc.push( { a: sortField } );
         expR.push( { a: sortField, id1: j * 5 + i, id2: -( j * 5 + i ) } );
         sortField++;
      }
      dbcl.insert( doc );
   }
   var actR = dbcl.find().sort( { a: 1 } );
   checkRec( actR, expR );
   println( "---check insert record not set increment field success" );

   for( var j = 0; j < 100; j++ )
   {
      var doc = [];
      for( var i = 0; i < 5; i++ )
      {
         doc.push( { a: sortField, id1: i, id2: i } );
         expR.push( { a: sortField, id1: i, id2: i } );
         sortField++;
      }
      dbcl.insert( doc );
   }
   var actR = dbcl.find().sort( { a: 1 } );
   checkRec( actR, expR );
   println( "---check insert set increment field success" );

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
