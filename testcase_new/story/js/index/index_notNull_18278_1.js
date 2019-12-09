/******************************************************************************
*@Description : seqDB-18278:创建复合索引，指定NotNull，索引键字段覆盖所有数据类型 
*               cover all type
*@Author      : 2019-5-6  XiaoNi Huang
******************************************************************************/

main();
function main ()
{
   var clName = "cl_18278_1";
   var indexName = "idx";
   var indexKey = { num: 1, a: 1, b: -1, c: 1, d: -1, e: 1, f: -1, g: 1, h: -1, i: 1, j: -1, k: 1, l: -1, m: 1, n: -1, o: 1, p: -1 };
   var recs1 = [{ num: 1, a: 2147483647, b: 9223372036854775807, c: 1.7E+308, d: { $decimal: "123.456" }, e: "test", f: { obj: "" }, g: { $oid: "100000009010000000901000" }, h: true, i: { $date: "2019-01-01" }, j: { $timestamp: "2019-01-01-01.00.00.000000" }, k: { $binary: "aGVsbG8gd29ybGQ=", "$type": "1" }, l: { $regex: "^a", $options: "i" }, m: [1, 2], n: { $minKey: 1 }, o: { $maxKey: 1 }, p: "lastFieldExistAndNotNull" }];
   var recs2 =
      [{ num: 2, a: 2147483647, b: 9223372036854775807, c: 1.7E+308, d: { $decimal: "123.456" }, e: "test", f: { obj: "" }, g: { $oid: "100000009010000000901000" }, h: true, i: { $date: "2019-01-01" }, j: { $timestamp: "2019-01-01-01.00.00.000000" }, k: { $binary: "aGVsbG8gd29ybGQ=", "$type": "1" }, l: { $regex: "^a", $options: "i" }, m: [1, 2], n: { $minKey: 1 }, o: { $maxKey: 1 }, p: null },
      { num: 3, a: 2147483647, b: 9223372036854775807, c: 1.7E+308, d: { $decimal: "123.456" }, e: "test", f: { obj: "" }, g: { $oid: "100000009010000000901000" }, h: true, i: { $date: "2019-01-01" }, j: { $timestamp: "2019-01-01-01.00.00.000000" }, k: { $binary: "aGVsbG8gd29ybGQ=", "$type": "1" }, l: { $regex: "^a", $options: "i" }, m: [1, 2], n: { $minKey: 1 }, o: { $maxKey: 1 } }];

   // ready cl
   commDropCL( db, COMMCSNAME, clName, true, true,
      "Failed to drop CL in the pre-condition." );
   var cl = commCreateCL( db, COMMCSNAME, clName, -1, true, true, false,
      "Failed to create CL." );

   /**************************** test1, create composite index[ NotNull:true ] -> insert **********************/
   var NotNull = true;
   println( "\n---Test1, create composite index[ NotNull:" + NotNull + " ] -> insert." );
   cl.createIndex( indexName, indexKey, { NotNull: NotNull } );

   cl.insert( recs1 );
   for( i = 0; i < recs2.length; i++ ) 
   {
      try
      {
         cl.insert( recs2[i] );
      }
      catch( e ) 
      {
         if( e !== -339 )
         {
            throw buildException( "checkResult", null, "", -339, "  " + e );
         }
      }
   }

   checkIndex( cl, indexName, NotNull );
   checkRecords( cl, recs1 );

   // clean index
   cl.dropIndex( indexName );
   cl.remove();


   /**************************** test2, create composite index[ NotNull:false ] -> insert **********************/
   var NotNull = false;
   println( "\n---Test2, create composite index[ NotNull:" + NotNull + " ] -> insert." );
   cl.createIndex( indexName, indexKey, { NotNull: NotNull } );
   checkIndex( cl, indexName, NotNull );

   println( "---insert recs1." );
   cl.insert( recs1 );
   checkRecords( cl, recs1 );

   cl.remove();

   println( "---insert recs2." );
   cl.insert( recs2 );
   checkRecords( cl, recs2 );

   // clean index
   cl.dropIndex( indexName );
   cl.remove();


   /****************** test3, create composite index[ NotNull:true ] -> insert[ a:null/exist ] ******************/
   var NotNull = true;
   println( "\n---Test1, create composite index[ NotNull:" + NotNull + " ] -> insert[ a:null/exist ]." );
   cl.createIndex( indexName, { a: 1, b: -1 }, { NotNull: NotNull } );

   var invRecs = [{ b: 1 }, { a: null, b: 2 }];
   for( i = 0; i < invRecs.length; i++ ) 
   {
      try
      {
         cl.insert( invRecs[i] );
      }
      catch( e ) 
      {
         if( e !== -339 )
         {
            throw buildException( "checkResult", null, "", -339, "  " + e );
         }
      }
   }

   checkIndex( cl, indexName, NotNull );
   var cnt = cl.count();
   if( Number( cnt ) !== 0 )
   {
      throw buildException( "checkResult", null, "", Number( cnt ), "  0" );
   }

   // clean index
   cl.dropIndex( indexName );
   cl.remove();

   // clean env
   commDropCL( db, COMMCSNAME, clName, false, false,
      "Failed to drop CL in the end-condition" );
}

function checkIndex ( cl, indexName, expNot ) 
{
   var indexDef = cl.getIndex( indexName ).toObj().IndexDef;
   var actNot = indexDef.NotNull;
   if( actNot !== expNot )
   {
      throw buildException( "checkResult", null, "", expNot, "  " + actNot );
   }
}

function checkRecords ( cl, expRecs ) 
{
   println( "---Check results." );
   var rc = cl.find( {}, { _id: { $include: 0 } } ).sort( { num: 1 } );
   var actRecs = new Array();
   while( tmpRecs = rc.next() )
   {
      actRecs.push( tmpRecs.toObj() );
   }

   if( JSON.stringify( expRecs ) !== JSON.stringify( actRecs ) )
   {
      throw buildException( "checkResult", null, "", JSON.stringify( expRecs ), "  " + JSON.stringify( actRecs ) );
   }
}