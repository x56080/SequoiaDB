/******************************************************************************
*@Description : seqDB-18278:创建复合索引，指定NotNull，索引键字段覆盖所有数据类型 
*               string length
                Non-standard format timestamp
*@Author      : 2019-5-6  XiaoNi Huang
******************************************************************************/

main();
function main ()
{
   var clName = "cl_18278_2";
   var indexName = "idx";
   var indexKey = { a: 1, b: -1 };

   var valRecs = [{ a: "", b: 1 }, { a: getRandomString( 1023 ), b: 2 }, { a: getRandomString( 1024 ), b: 2 }, { a: getRandomString( 1025 ), b: 2 }];
   var invRecs = [{ a: "" }, { a: getRandomString( 1023 ), b: null }, { a: getRandomString( 1024 ), b: null }, { a: getRandomString( 1025 ), b: null }];

   var nesArr = [{ a: [{ a1: 1 }, { a2: 2 }, { a3: 2 }, { a4: 2 }, { a5: 2 }, { a6: 2 }, { a7: 2 }, { a8: 2 }, { a9: 2 }, { a10: 2 }, { a11: 2 }, { a12: 2 }, { a13: 2 }, { a14: 2 }, { a15: 2 }, { a16: 2 }, { a17: 2 }, { a18: 2 }, { a19: 2 }, { a20: 2 }], b: 1 }];

   var nesObj = [{ a: { a2: { a3: { a4: { a5: { a6: { a7: { a8: { a9: { a10: { a11: { a12: { a13: { a14: { a15: { a16: { a17: { a18: { a19: { a20: "nestedObj" } } } } } } } } } } } } } } } } } } }, b: 1 }];

   // ready cl
   commDropCL( db, COMMCSNAME, clName, true, true,
      "Failed to drop CL in the pre-condition." );
   var cl = commCreateCL( db, COMMCSNAME, clName, {}, true, false,
      "Failed to create CL." );

   /************************* test1, create composite index[ NotNull:true ] -> insert[ string ] *******************/
   var NotNull = true;
   println( "\n---Test1, create composite index[ NotNull:" + NotNull + " ] -> insert[ string ]." );
   cl.createIndex( indexName, indexKey, { NotNull: NotNull } );

   cl.insert( valRecs );
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
   checkRecords( cl, valRecs );

   // clean index
   cl.dropIndex( indexName );
   cl.remove();


   /************************* test2, create composite index[ NotNull:false ] -> insert[ string ] *******************/
   var NotNull = false;
   println( "\n---Test2, create composite index[ NotNull:" + NotNull + " ] -> insert[ string ]." );
   cl.createIndex( indexName, indexKey, { NotNull: NotNull } );
   checkIndex( cl, indexName, NotNull );

   println( "---insert recs1." );
   cl.insert( valRecs );
   checkRecords( cl, valRecs );

   cl.remove();

   println( "---insert invRecs." );
   cl.insert( invRecs );
   checkRecords( cl, invRecs );

   // clean index
   cl.dropIndex( indexName );
   cl.remove();


   /****************** test3, create composite index[ NotNull:true ] -> insert[ a:timestamp ] ******************/
   var NotNull = true;
   println( "\n---Test3, create composite index[ NotNull:" + NotNull + " ] -> insert[ a:timestamp ]." );
   cl.createIndex( indexName, indexKey, { NotNull: NotNull } );
   checkIndex( cl, indexName, NotNull );

   var v1 = "1901-12-31T15:54:03.000Z";
   var v2 = "2037-12-31T23:59:59.999+0800";
   var timestempRecs = [{ a: { $timestamp: v1 }, b: 1 },
   { a: { $timestamp: v2 }, b: 2 }];
   cl.insert( timestempRecs );

   // check results
   var localtime1 = turnLocaltime( v1, '%Y-%m-%d-%H.%M.%S.000000' );
   var expRecs = [{ a: { $timestamp: localtime1 }, b: 1 },
   { a: { $timestamp: "2037-12-31-23.59.59.999000" }, b: 2 }];
   checkRecords( cl, expRecs );

   cl.remove();


   /****************** test4, create composite index[ NotNull:true ] -> insert[ a:nested array ] ******************/
   var NotNull = true;
   println( "\n---Test4, create composite index[ NotNull:" + NotNull + " ] -> insert[ a:nested array ]." );
   cl.insert( nesArr );
   checkRecords( cl, nesArr );

   cl.remove();


   /****************** test5, create composite index[ NotNull:true ] -> insert[ a:nested obj ] ******************/
   var NotNull = true;
   println( "\n---Test5, create composite index[ NotNull:" + NotNull + " ] -> insert[ a:nested obj ]." );
   cl.insert( nesObj );
   checkRecords( cl, nesObj );

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
   var rc = cl.find( {}, { _id: { $include: 0 } } ).sort( { b: 1 } );
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

function getRandomString ( strLen ) 
{
   var str = "";
   for( var i = 0; i < strLen; i++ )
   {
      var ascii = getRandomInt( 48, 127 ); // '0' -- '~'
      var c = String.fromCharCode( ascii );
      str += c;
   }
   return str;
}

function getRandomInt ( min, max ) // [min, max)
{
   var range = max - min;
   var value = min + parseInt( Math.random() * range );
   return value;
}