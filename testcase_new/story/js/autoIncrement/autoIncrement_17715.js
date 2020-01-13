/***************************************************************************
@Description :seqDB-17715:increment为正值，在序列未使用时（无缓存状态）插入值
@Modify list :
              2019-1-24  zhaoyu  Create
****************************************************************************/
function main ()
{
   var coordNodes = getCoordNodeNames();
   var coordNum = coordNodes.length;
   if( commIsStandalone( db ) || coordNum !== 3 )
   {
      println( "Deploy is standalone or coord num !=3" );
      return;
   }
   var sortField = 0;
   var increment = 1;
   var currentValue = 1
   var cacheSize = 1000;
   var acquireSize = 11;
   var clName = COMMCLNAME + "_17715";
   commDropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName, { AutoIncrement: { Field: "id", AcquireSize: acquireSize, CacheSize: cacheSize } } );
   commCreateIndex( dbcl, "id", { id: 1 }, { Unique: true }, true );

   var expR = [];
   var cl = new Array();
   var coord = new Array();
   for( var k = 0; k < coordNum; k++ )
   {
      coord[k] = new Sdb( coordNodes[k] );
      println( "coord:" + coord[k] );
      cl[k] = coord[k].getCS( COMMCSNAME ).getCL( clName );
   }

   //coordB指定自增字段插入记录，本coord缓存更新为[118,128]
   cl[1].insert( { a: sortField, id: 117 } );
   expR.push( { a: sortField, id: 117 } );
   println( "---insert set autoIncrement success" );

   //coordA插入记录，不指定自增字段，本coord缓存更新为[129,139]
   for( var i = 0; i < 2; i++ )
   {
      cl[0].insert( { a: sortField } );
      expR.push( { a: sortField, id: 129 + i } );
      sortField++;
   }
   println( "---coordA insert success" );

   //coordB插入记录，不指定自增字段
   for( var i = 0; i < 2; i++ )
   {
      cl[1].insert( { a: sortField } );
      expR.push( { a: sortField, id: 118 + i } );
      sortField++;
   }
   println( "---coordB insert success" );

   //coordC插入记录，不指定自增字段,本coord缓存更新为[140,150]
   for( var i = 0; i < 2; i++ )
   {
      cl[2].insert( { a: sortField } );
      expR.push( { a: sortField, id: 140 + i } );
      sortField++;
   }
   println( "---coordC insert success" );

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
