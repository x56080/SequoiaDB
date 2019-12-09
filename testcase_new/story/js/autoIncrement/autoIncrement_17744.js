/******************************************************************************
@Description :    seqDB-17744: increment为负值，超大范围调整 
@Modify list :   2018-1-29    Zhao Xiaoni  Init
******************************************************************************/
function main ()
{
   var coordNodes = getCoordNodeNames();
   if( coordNodes.length < 3 || commIsStandalone( db ) )
   {
      println( "Deploy is standalone or coord nodes is less than 3!" );
      return;
   }

   var clName = COMMCLNAME + "_17744";
   var increment = -1;
   var acquireSize = 5;
   var maxValue = { "$numberLong": "9223372036854775807" }

   commDropCL( db, COMMCSNAME, clName );

   var dbcl = commCreateCLByOption( db, COMMCSNAME, clName, {
      AutoIncrement: {
         Field: "id", Increment: increment,
         AcquireSize: acquireSize, MaxValue: maxValue
      }
   } );
   commCreateIndex( dbcl, "a", { id: 1 }, true );

   dbcl.setAttributes( { AutoIncrement: { Field: "id", CurrentValue: maxValue } } );

   //连接所有coord插入部分记录,coord缓存分别为[{ "$numberLong" : "9223372036854775802" },{ "$numberLong" : "9223372036854775806" }],
   //[{ "$numberLong" : "9223372036854775797" },{ "$numberLong" : "9223372036854775801" }],
   //[{ "$numberLong" : "9223372036854775792" },{ "$numberLong" : "9223372036854775796" }]
   var expRecs = [];
   var cl = new Array();
   var coord = new Array();
   for( var i = 0; i < coordNodes.length; i++ )
   {
      coord[i] = new Sdb( coordNodes[i] );
      cl[i] = coord[i].getCS( COMMCSNAME ).getCL( clName );
      cl[i].insert( { a: i } );
   }

   expRecs.push( { a: 0, id: { "$numberLong": "9223372036854775806" } } );
   expRecs.push( { a: 1, id: { "$numberLong": "9223372036854775801" } } );
   expRecs.push( { a: 2, id: { "$numberLong": "9223372036854775796" } } );

   var insertR1 = { a: 4, id: { "$numberLong": "-9223372036854775808" } }
   cl[1].insert( insertR1 );
   expRecs.push( insertR1 );

   //coordA插入记录，消耗完本coord的缓存[{ "$numberLong" : "9223372036854775802" },{ "$numberLong" : "9223372036854775806" }]
   for( var i = 0; i < 4; i++ )
   {
      cl[0].insert( { a: i } );
   }
   expRecs.push( { a: 0, id: { $numberLong: "9223372036854775805" } } );
   expRecs.push( { a: 1, id: { $numberLong: "9223372036854775804" } } );
   expRecs.push( { a: 2, id: { $numberLong: "9223372036854775803" } } );
   expRecs.push( { a: 3, id: { $numberLong: "9223372036854775802" } } );
   println( "---coordA insert success" );

   //coordA插入记录，插入失败，超出序列值返回
   try
   {
      cl[0].insert( { a: 1 } );
      throw new Error( "NEED_ERROR" );
   } catch( e )
   {
      if( e !== -325 )
      {
         throw new Error( e );
      }
   }
   println( "---coordA get cache success" );

   //coordB插入记录，插入失败，超出序列值范围
   try
   {
      cl[1].insert( { a: 1 } );
      throw new Error( "NEED_ERROR" );
   } catch( e )
   {
      if( e !== -325 )
      {
         throw new Error( e );
      }
   }
   println( "---coordB get cache success" );

   //coordC插入记录，消耗完本coord的缓存[{ "$numberLong" : "922337203685477601" },{ "$numberLong" : "9223372036854775705" }]
   for( var i = 0; i < 4; i++ )
   {
      cl[2].insert( { a: i } );
   }
   expRecs.push( { a: 0, id: { $numberLong: "9223372036854775795" } } );
   expRecs.push( { a: 1, id: { $numberLong: "9223372036854775794" } } );
   expRecs.push( { a: 2, id: { $numberLong: "9223372036854775793" } } );
   expRecs.push( { a: 3, id: { $numberLong: "9223372036854775792" } } );
   println( "---coordC insert success" );

   //coordC插入记录，插入失败，超出序列值范围
   try
   {
      cl[2].insert( { a: 1 } );
      throw new Error( "NEED_ERROR" );
   } catch( e )
   {
      if( e !== -325 )
      {
         throw new Error( e );
      }
   }
   println( "---coordC get cache success" );

   var rc = dbcl.find().sort( { _id: 1 } );
   checkRec( rc, expRecs.sort( compare( "_id" ) ) );

   commDropCL( db, COMMCSNAME, clName );
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
