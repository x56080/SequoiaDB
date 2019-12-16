/***************************************************************************
@Description :seqDB-17721:increment为正值，不允许翻转，插入值是序列的MaxValue 
@Modify list :
              2019-1-25  zhaoyu  Create
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
   var clName = COMMCLNAME + "_17721";
   commDropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName, { AutoIncrement: { Field: "id", AcquireSize: acquireSize, CacheSize: cacheSize, Cycled: false } } );
   commCreateIndex( dbcl, "id", { id: 1 }, true, true );

   var expR = [];
   var cl = new Array();
   var coord = new Array();
   for( var k = 0; k < coordNum; k++ )
   {
      coord[k] = new Sdb( coordNodes[k] );
      println( "coord:" + coord[k] );
      cl[k] = coord[k].getCS( COMMCSNAME ).getCL( clName );
      //连接所有coord插入部分记录,coord缓存分别为[1,11],[12,22],[23,33]
      var doc = [];
      for( var i = 0; i < 3; i++ )
      {
         doc.push( { a: sortField } );
         expR.push( { a: sortField, id: currentValue + Math.ceil( 3 / acquireSize ) * acquireSize * increment * k + increment * i } );
         sortField++;
      }
      cl[k].insert( doc );
   }
   println( "---prepare insert success" );

   //coordB指定自增字段插入记录，插入值是序列的maxValue,coordB丢弃本coord的缓存，重新从catalog上获取新缓存,[1,11]
   cl[1].insert( { a: sortField, id: { $numberLong: "9223372036854775807" } } );
   expR.push( { a: sortField, id: { $numberLong: "9223372036854775807" } } );
   println( "---insert set autoIncrement success" );

   //coordA插入记录，消耗完本coord的缓存，[1,11]
   for( var i = 0; i < 8; i++ )
   {
      cl[0].insert( { a: sortField } );
      expR.push( { a: sortField, id: 4 + i } );
      sortField++;
   }
   println( "---coordA insert success" );

   var actR = dbcl.find().sort( { a: 1 } );
   checkRec( actR, expR );
   println( "---check insert success" );

   //为了避免唯一键重复，删除集合中已有记录，方便校验后续生成的自增字段值
   dbcl.remove();
   expR = [];

   //coordA插入记录，插入失败
   try
   {
      cl[0].insert( { a: sortField } );
      throw new Error( "NEED_ERROR" );
   } catch( e )
   {
      if( e !== -325 )
      {
         throw new Error( e );
      }
   }
   println( "---coordA get cache success" );

   //coordB插入记录，插入失败
   try
   {
      cl[1].insert( { a: sortField } );
      throw new Error( "NEED_ERROR" );
   } catch( e )
   {
      if( e !== -325 )
      {
         throw new Error( e );
      }
   }
   println( "---coordB get cache success" );

   //coordC插入记录，消耗完本coord的缓存，[23,33]
   for( var i = 0; i < 8; i++ )
   {
      cl[2].insert( { a: sortField } );
      expR.push( { a: sortField, id: 26 + i } );
      sortField++;
   }
   println( "---coordC insert success" );

   //coordC插入记录，插入失败
   try
   {
      cl[2].insert( { a: sortField } );
      throw new Error( "NEED_ERROR" );
   } catch( e )
   {
      if( e !== -325 )
      {
         throw new Error( e );
      }
   }
   println( "---coordC get cache success" );

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
