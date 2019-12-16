/***************************************************************************
@Description :seqDB-16000 :��������ʱ��ָ����ʼֵ/��Сֵ/���ֵ�����ݼ��������ֶ�
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

   var clName = COMMCLNAME + "_16000";
   commDropCL( db, COMMCSNAME, clName, true, true );

   var increment = -1;
   var cacheSize = 10;
   var acquireSize = 1;
   var fieldName = "id";
   var minValue = -2000;
   var maxValue = 10000;
   var startValue = 1000;
   var dbcl = commCreateCL( db, COMMCSNAME, clName, { AutoIncrement: { Field: fieldName, Increment: increment, CacheSize: cacheSize, AcquireSize: acquireSize, MinValue: minValue, MaxValue: maxValue, StartValue: startValue } } );

   var clID = getCLID( COMMCSNAME, clName );
   var clSequenceName = "SYS_" + clID + "_" + fieldName + "_SEQ";
   var expArr = [{ Field: fieldName, SequenceName: clSequenceName }];
   checkAutoIncrementonCL( COMMCSNAME, clName, expArr );
   println( "---check autoIncrement success" );

   var expObj = { Increment: increment, CacheSize: cacheSize, AcquireSize: acquireSize, CurrentValue: startValue, MinValue: minValue, MaxValue: maxValue, StartValue: startValue };
   checkSequence( clSequenceName, expObj );
   println( "---check sequence success" );

   var doc = [];
   var expR = [];
   for( var i = 0; i < 2000; i++ )
   {
      doc.push( { a: sortField } );
      expR.push( { a: sortField, id: startValue + increment * i } );
      sortField++;
   }
   dbcl.insert( doc );

   var actR = dbcl.find().sort( { a: 1 } );
   checkRec( actR, expR );
   println( "---check insert when set startValue = 1000 success" );

   dbcl.dropAutoIncrement( fieldName );
   var increment = -1;
   var minValue = -10000;
   var maxValue = -10;
   var startValue = -1000;
   dbcl.createAutoIncrement( { Field: fieldName, Increment: increment, CacheSize: cacheSize, AcquireSize: acquireSize, MinValue: minValue, MaxValue: maxValue, StartValue: startValue } );

   var clID = getCLID( COMMCSNAME, clName );
   var clSequenceName = "SYS_" + clID + "_" + fieldName + "_SEQ";
   var expArr = [{ Field: fieldName, SequenceName: clSequenceName }];
   checkAutoIncrementonCL( COMMCSNAME, clName, expArr );
   println( "---check autoIncrement when set startValue = -1000 success" );

   var expObj = { Increment: increment, CacheSize: cacheSize, AcquireSize: acquireSize, CurrentValue: startValue, MinValue: minValue, MaxValue: maxValue, StartValue: startValue };
   checkSequence( clSequenceName, expObj );
   println( "---check sequence when set startValue = -1000 success" );

   var doc = [];
   for( var i = 0; i < 2000; i++ )
   {
      doc.push( { a: sortField } );
      expR.push( { a: sortField, id: startValue + increment * i } );
      sortField++;
   }
   dbcl.insert( doc );

   var actR = dbcl.find().sort( { a: 1 } );
   checkRec( actR, expR );
   println( "---check insert when set startValue = -1000 success" );

   dbcl.dropAutoIncrement( fieldName );
   var increment = -1;
   var minValue = -5000;
   var maxValue = 1000;
   var startValue = 0;
   dbcl.createAutoIncrement( { Field: fieldName, Increment: increment, CacheSize: cacheSize, AcquireSize: acquireSize, MinValue: minValue, MaxValue: maxValue, StartValue: startValue } );

   var clID = getCLID( COMMCSNAME, clName );
   var clSequenceName = "SYS_" + clID + "_" + fieldName + "_SEQ";
   var expArr = [{ Field: fieldName, SequenceName: clSequenceName }];
   checkAutoIncrementonCL( COMMCSNAME, clName, expArr );
   println( "---check autoIncrement when set startValue = 0 success" );

   var expObj = { Increment: increment, CacheSize: cacheSize, AcquireSize: acquireSize, CurrentValue: startValue, MinValue: minValue, MaxValue: maxValue, StartValue: startValue };
   checkSequence( clSequenceName, expObj );
   println( "---check sequence when set startValue = 0 success" );

   var doc = [];
   for( var i = 0; i < 2000; i++ )
   {
      doc.push( { a: sortField } );
      expR.push( { a: sortField, id: startValue + increment * i } );
      sortField++;
   }
   dbcl.insert( doc );

   var actR = dbcl.find().sort( { a: 1 } );
   checkRec( actR, expR );
   println( "---check insert when set startValue = 0 success" );

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
