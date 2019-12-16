/***************************************************************************
@Description :seqDB-15937 :不同coord不指定自增字段插入记录,递增
@Modify list :
              2018-10-15  zhaoyu  Create
****************************************************************************/
var sortField = 0;
function main ()
{
   if( commIsStandalone( db ) )
   {
      println( "Deploy is standalone" );
      return;
   };

   var clName = COMMCLNAME + "_15937_1";
   commDropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName, { AutoIncrement: { Field: "id", AcquireSize: 10 } } );
   commCreateIndex( dbcl, "id", { id: 1 }, true, true );

   var coordNodes = getCoordNodeNames();
   var coordNum = coordNodes.length;
   var expR = [];
   for( var j = 0; j < 2; j++ )
   {
      for( var k = 0; k < coordNum; k++ )
      {
         var coord = new Sdb( coordNodes[k] );
         //coord.invalidateCache();
         var cl = coord.getCS( COMMCSNAME ).getCL( clName );
         var doc = [];
         for( var i = 1; i < 101; i++ )
         {
            doc.push( { a: sortField, b: i, c: i + "test" } );
            expR.push( { a: sortField, b: i, c: i + "test", id: j * coordNum * 100 + 100 * k + i } );
            sortField++;
         }
         cl.insert( doc );
         coord.close();
      }

   }
   var actR = dbcl.find().sort( { a: 1 } );
   checkRec( actR, expR );
   println( "---check insert success" );

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
