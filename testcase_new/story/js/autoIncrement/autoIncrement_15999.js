/***************************************************************************
@Description :seqDB-15999 :��������ʱ�����������������ֶ� 
@Modify list :
              2018-10-24  zhaoyu  Create
****************************************************************************/
var sortField = 0;
function main ()
{
   if( commIsStandalone( db ) )
   {
      println( "Deploy is standalone" );
      return;
   }

   var clName = COMMCLNAME + "_15999";
   commDropCL( db, COMMCSNAME, clName, true, true );

   var increment = 1;
   var cacheSize = 10;
   var acquireSize = 1;
   var fieldName = "id";
   var dbcl = commCreateCL( db, COMMCSNAME, clName, { AutoIncrement: { Field: fieldName, Increment: increment, CacheSize: cacheSize, AcquireSize: acquireSize } } );

   var clID = getCLID( COMMCSNAME, clName );
   var clSequenceName = "SYS_" + clID + "_" + fieldName + "_SEQ";
   var expArr = [{ Field: fieldName, SequenceName: clSequenceName }];
   checkAutoIncrementonCL( COMMCSNAME, clName, expArr );
   println( "---check autoIncrement success" );

   var expObj = { Increment: increment, CacheSize: cacheSize, AcquireSize: acquireSize };
   checkSequence( clSequenceName, expObj );
   println( "---check sequence success" );

   var doc = [];
   var expR = [];
   for( var i = 0; i < 2000; i++ )
   {
      doc.push( { a: sortField } );
      expR.push( { a: sortField, id: 1 + increment * i } );
      sortField++;
   }
   dbcl.insert( doc );

   var actR = dbcl.find().sort( { a: 1 } );
   checkRec( actR, expR );
   println( "---check insert when set increment = 1 success" );

   dbcl.dropAutoIncrement( fieldName );
   var increment = 2147483647;
   dbcl.createAutoIncrement( { Field: fieldName, Increment: increment, CacheSize: cacheSize, AcquireSize: acquireSize } );

   var clID = getCLID( COMMCSNAME, clName );
   var clSequenceName = "SYS_" + clID + "_" + fieldName + "_SEQ";
   var expArr = [{ Field: fieldName, SequenceName: clSequenceName }];
   checkAutoIncrementonCL( COMMCSNAME, clName, expArr );
   println( "---check autoIncrement when set increment 2147483647 success" );

   var expObj = { Increment: increment, CacheSize: cacheSize, AcquireSize: acquireSize };
   checkSequence( clSequenceName, expObj );
   println( "---check sequence when set increment 2147483647 success" );

   var doc = [];
   for( var i = 0; i < 2000; i++ )
   {
      doc.push( { a: sortField } );
      expR.push( { a: sortField, id: 1 + increment * i } );
      sortField++;
   }
   dbcl.insert( doc );

   var actR = dbcl.find().sort( { a: 1 } );
   checkRec( actR, expR );
   println( "---check insert when set increment = 2147483647 success" );

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
