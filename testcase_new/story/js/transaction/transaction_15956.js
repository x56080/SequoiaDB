/***************************************************************************
@Description : seqDB-15956:事务中操作自增字段的集合，事务提交功能
@Modify list :
2018-10-30  zhaoyu  Create
****************************************************************************/
function main ()
{
   if( commIsStandalone( db ) )
   {
      println( "Deploy is standalone" );
      return;
   }

   var clName1 = COMMCLNAME + "_15956_1";
   var clName2 = COMMCLNAME + "_15956_2";
   commDropCL( db, COMMCSNAME, clName1, true, true );
   commDropCL( db, COMMCSNAME, clName2, true, true );

   var dbcl1 = commCreateCL( db, COMMCSNAME, clName1, { AutoIncrement: { Field: "id" } } );

   db.transBegin();
   var dbcl2 = commCreateCL( db, COMMCSNAME, clName2, { AutoIncrement: { Field: "id" } } );
   var doc = [];
   var expR = [];
   for( var i = 0; i < 100; i++ )
   {
      doc.push( { a: i } );
      expR.push( { a: i, id: 1 + i } );

   }
   dbcl1.insert( doc );
   dbcl2.insert( doc );
   db.transCommit();

   var actR = dbcl1.find().sort( { _id: 1 } );
   checkRec( actR, expR );
   println( "---check insert dbcl1, commit success" );

   var actR = dbcl2.find().sort( { _id: 1 } );
   checkRec( actR, expR );
   println( "---check insert dbcl2, commit success" );

   dbcl1.insert( { a: "insert" } );
   dbcl2.insert( { a: "insert" } );
   expR.push( { a: "insert", id: 101 } );

   var actR = dbcl1.find().sort( { _id: 1 } );
   checkRec( actR, expR );
   println( "---check insert dbcl1 after transcommit success" );

   var actR = dbcl2.find().sort( { _id: 1 } );
   checkRec( actR, expR );
   println( "---check insert dbcl2 after transcommit success" );

   commDropCL( db, COMMCSNAME, clName1, true, true );
   commDropCL( db, COMMCSNAME, clName2, true, true );
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
