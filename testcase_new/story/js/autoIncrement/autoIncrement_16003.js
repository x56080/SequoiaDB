/***************************************************************************
@Description :seqDB-16003 :��������ʱ��ָ��coordһ�λ�ȡ���������������ֶ� 
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

   var clName = COMMCLNAME + "_16003";
   commDropCL( db, COMMCSNAME, clName, true, true );

   var increment = 11;
   var cacheSize = 1000;
   var acquireSize = 10;
   var fieldName = "id";
   var dbcl = commCreateCLByOption( db, COMMCSNAME, clName, { AutoIncrement: { Field: fieldName, Increment: increment, CacheSize: cacheSize, AcquireSize: acquireSize } } );

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
   println( "---check insert when set cacheSize%acquireSize==0 success" );

   dbcl.dropAutoIncrement( fieldName );
   var increment = 10;
   var cacheSize = 1000;
   var acquireSize = 11;
   dbcl.createAutoIncrement( { Field: fieldName, Increment: increment, CacheSize: cacheSize, AcquireSize: acquireSize } );
   var expArr = [{ Field: fieldName, SequenceName: clSequenceName }];
   checkAutoIncrementonCL( COMMCSNAME, clName, expArr );
   println( "---check autoIncrement success" );

   var expObj = { Increment: increment, CacheSize: cacheSize, AcquireSize: acquireSize };
   checkSequence( clSequenceName, expObj );
   println( "---check sequence success" );

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
   println( "---check insert when set cacheSize%acquireSize!=0 success" );

   dbcl.dropAutoIncrement( fieldName );
   var increment = 10;
   var cacheSize = 111;
   var acquireSize = 111;
   dbcl.createAutoIncrement( { Field: fieldName, Increment: increment, CacheSize: cacheSize, AcquireSize: acquireSize } );
   var expArr = [{ Field: fieldName, SequenceName: clSequenceName }];
   checkAutoIncrementonCL( COMMCSNAME, clName, expArr );
   println( "---check autoIncrement success" );

   var expObj = { Increment: increment, CacheSize: cacheSize, AcquireSize: acquireSize };
   checkSequence( clSequenceName, expObj );
   println( "---check sequence success" );

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
   println( "---check insert when set cacheSize=acquireSize success" );

   dbcl.dropAutoIncrement( fieldName );
   var increment = 10;
   var cacheSize = 111;
   var acquireSize = 112;
   try
   {
      dbcl.createAutoIncrement( { Field: fieldName, Increment: increment, CacheSize: cacheSize, AcquireSize: acquireSize } );
      throw "NEED_CREATE_ERR";
   } catch( e )
   {
      if( -6 !== e )
      {
         throw new Error( e );
      }
   }

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
