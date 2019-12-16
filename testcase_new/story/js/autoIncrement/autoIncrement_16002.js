/***************************************************************************
@Description :seqDB-16002 :��������ʱ��ָ��catalogһ�λ������������������ֶ� 
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

   var clName = COMMCLNAME + "_16002";
   commDropCL( db, COMMCSNAME, clName, true, true );

   var increment = -2;
   var cacheSize = 2147483647;
   var acquireSize = 1;
   var fieldName = "id";
   var minValue = -2147483647;
   var maxValue = 2147483647;
   var startValue = 0;
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
   println( "---check insert when set cacheSize success" );

   dbcl.dropAutoIncrement( fieldName );
   var increment = -3;
   var cacheSize = 2147483647;
   var acquireSize = 1;
   var fieldName = "id";
   var minValue = -2147483647;
   var maxValue = 2147483647;
   var startValue = 0;
   try
   {
      dbcl.createAutoIncrement( { Field: fieldName, Increment: increment, CacheSize: cacheSize, AcquireSize: acquireSize, MinValue: minValue, MaxValue: maxValue, StartValue: startValue } );
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
