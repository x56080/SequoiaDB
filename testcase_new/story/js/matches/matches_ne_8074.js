/************************************************************************
*@Description:   seqDB-8073:使用$ne查询，目标字段为数组且元素为数值型，走索引查询
                     data type: array/object;   index:{b:1}
*@Author:  2016/5/16  xiaoni huang
************************************************************************/
main();

function main ()
{
   try
   {
      var clName = COMMCLNAME + "_matches8074";
      var indexName = CHANGEDPREFIX + "_index";

      var cl = readyCL( clName );
      createIndex( cl, indexName );

      var rawData = [{ int: [-2147483648, 2147483647] },
      { long: ["-9223372036854775808", "9223372036854775807"] }];
      insertRecs( cl, rawData );

      var arrRc = findRecs( cl, rawData, "array" );
      var objRc = findRecs( cl, rawData, "object" );

      checkResult( arrRc, objRc, rawData, indexName );

      cleanCL( clName );
   }
   catch( e )
   {
      throw e;
   }
}

function createIndex ( cl, indexName )
{
   println( "\n---Begin to create index." );

   cl.createIndex( indexName, { b: 1 } );
}

function insertRecs ( cl, rawData )
{
   println( "\n---Begin to insert records." );
   cl.insert( {
      a: 0, b: [{ b1: rawData[0]["int"][0] },
      { b2: rawData[0]["int"][1] }]
   } );
   cl.insert( {
      a: 1, b: {
         b1: { c1: { $numberLong: rawData[1]["long"][0] } },
         b2: { c2: { $numberLong: rawData[1]["long"][1] } }
      }
   } );
}

function findRecs ( cl, rawData, dataType )
{
   println( "\n---Begin to find records for dataType[" + dataType + "]." );

   if( dataType === "array" )
   {
      var rc = cl.find( {
         "b": {
            $ne: [{ b1: rawData[0]["int"][0] },
            { b2: rawData[0]["int"][1] }]
         }
      } ).sort( { a: 1 } );
   }
   else if( dataType === "object" )
   {
      var rc = cl.find( {
         b: {
            $ne: {
               b1: { c1: { $numberLong: rawData[1]["long"][0] } },
               b2: { c2: { $numberLong: rawData[1]["long"][1] } }
            }
         }
      } ).sort( { a: 1 } );
   }

   return rc;
}

function checkResult ( arrRc, objRc, rawData, indexName )
{
   println( "\n---Begin to check index." );

   //compare scanType: $ne with array should not generate predicates
   var tmpExp = arrRc.explain().current().toObj();
   if( tmpExp["ScanType"] !== "tbscan" )
   {
      throw buildException( "checkResult", null, "[compare index]",
         "[ScanType:tbscan]",
         "[ScanType:" + tmpExp["ScanType"] + ",IndexName:" + tmpExp["IndexName"] + "]" );
   }

   //-----------------------check result for dataType[array]---------------------
   println( "\n---Begin to check result for dataType[array]." );

   var findRtn = new Array();
   while( tmpRecs = arrRc.next() )    //arrRc
   {
      findRtn.push( tmpRecs.toObj() );
   }
   //compare number
   var expLen = 1;
   if( findRtn.length !== expLen )
   {
      throw buildException( "checkResult", null, "[compare number]",
         "[recsNum:" + expLen + "]",
         "[recsNum:" + findRtn.length + "]" );
   }
   //compare records  ---$ne: "array"
   if( findRtn[0]["b"]["b1"]["c1"]["$numberLong"].toString() !== rawData[1]["long"][0] ||   //"b.b1.c1":{$numberlong:"xxxxxx"}
      findRtn[0]["b"]["b2"]["c2"]["$numberLong"].toString() !== rawData[1]["long"][1] )  
   {
      println( "---The real results after the find by matches[$ne]: \n" + JSON.stringify( findRtn ) );
      throw buildException( "checkResult", null, "[compare records]",
         '["b.b1.c1":' + rawData[1]["long"][0]
         + ',"b.b2.c2":' + rawData[1]["long"][1] + "]",
         '["b.b1.c1":' + findRtn[0]["b"]["b1"]["c1"]["$numberLong"].toString()
         + ',"b.b2.c2":' + findRtn[0]["b"]["b2"]["c2"]["$numberLong"].toString() + "]" );
   }

   //-----------------------check result for dataType[object]---------------------
   println( "\n---Begin to check result for dataType[object]." );

   var findRtn = new Array();
   while( tmpRecs = objRc.next() )    //objRc
   {
      findRtn.push( tmpRecs.toObj() );
   }
   //compare number
   var expLen = 1;
   if( findRtn.length !== expLen )
   {
      throw buildException( "checkResult", null, "[compare number]",
         "[recsNum:" + expLen + "]",
         "[recsNum:" + findRtn.length + "]" );
   }
   //compare records  ---$ne: "object"
   if( findRtn[0]["b"][0]["b1"] !== rawData[0]["int"][0] ||   //expRecs--"b":[{"b1":-2147483648},{"b2":2147483647}]}]
      findRtn[0]["b"][1]["b2"] !== rawData[0]["int"][1] )  
   {
      println( "---The real results after the find by matches[$ne]: \n" + JSON.stringify( findRtn ) );
      throw buildException( "checkResult", null, "[compare records]",
         '["b.b1":' + rawData[0]["int"][0] + ',"b.b2":' + rawData[0]["int"][1] + "]",
         '["b.b1":' + findRtn[0]["b"][0]["b1"] + ',"b.b2":' + findRtn[0]["b"][1]["b1"] + "]" );
   }
}
