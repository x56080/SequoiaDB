/************************************************************************
*@Description:   seqDB-8092:使用$type查询，走索引查询
      base type:    16--int32; 1--double;    10--null; 2--string; 8--bool;   3--subObj; 4--array; 
      special type: 18--int64; 100--decimal; 7--oid;   11--regex; 5--binary; 9--date;  17--timestamp;
                    cover all data type
*@Author:  2016/5/21  xiaoni huang
*@bug: jira-8092/ jira-1219
************************************************************************/
main();

function main ()
{
   try
   {
      var clName = COMMCLNAME + "_matches8092";
      var indexName = CHANGEDPREFIX + "_index";
      var cl = readyCL( clName );
      createIndex( cl, indexName );

      //typeNum: 14
      var dataType = ["int", "double", "null", "string", "bool", "subObj", "array",
         "long", "decimal", "oid", "regex", "binary", "date", "timestamp"];
      var rawData = [{ int: -2147483648 },
      { double: -1.7E+308 },
      { null: null },
      { string: "test" },
      { bool: true },
      { subObj: { "0": { c: "test" } } },
      { array: [{ c: "test" }] },
      { long: { "$numberLong": "-9223372036854775808" } },
      { decimal: { "$decimal": "111.001" } },
      { oid: { "$oid": "123abcd00ef12358902300ef" } },
      { regex: { "$regex": "^rg", "$options": "i" } },
      { binary: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" } },
      { date: { "$date": "2038-01-18" } },
      { timestamp: { "$timestamp": "2038-01-18-23.59.59.999999" } }];
      insertRecs( cl, rawData, dataType );

      var numType = [16, 1, 10, 2, 8, 3, 4,
         18, 100, 7, 11, 5, 9, 17];
      var findRecsArray = findRecs( cl, dataType, numType );

      checkResult( cl, indexName, findRecsArray, rawData, dataType, numType );

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

function insertRecs ( cl, rawData, dataType )
{
   println( "\n---Begin to insert records." );

   for( i = 0; i < rawData.length; i++ )
   {
      cl.insert( { a: i, b: rawData[i][dataType[i]] } );
   }
}

function findRecs ( cl, dataType, numType )
{
   println( "\n---Begin to find records." );

   var findRecsArray = [];
   for( i = 0; i < numType.length; i++ )
   {
      println( "---Find for type[" + dataType[i] + "---" + numType[i] + "]." );

      var rc = cl.find( { b: { $type: 1, $et: numType[i] } }, { _id: { $include: 0 } } ).sort( { a: 1 } );
      var tmpArray = [];
      while( tmpRecs = rc.next() )
      {
         tmpArray.push( tmpRecs.toObj() );
      }
      //println(JSON.stringify(tmpArray));

      findRecsArray.push( tmpArray );
   }
   return findRecsArray;
}

function checkResult ( cl, indexName, findRecsArray, rawData, dataType, numType )
{
   //-------------------check index----------------------------
   println( "\n---Begin to check index." );

   //compare scanType
   var idx = cl.find( { b: { $type: 1, $et: 16 } } ).explain().current().toObj();
   if( idx["ScanType"] !== "tbscan" )
   {
      throw buildException( "checkResult", null, "[compare index]",
         "[ScanType:ixscan]",
         "[ScanType:" + idx["ScanType"] + "]" );
   }

   //-------------------check records----------------------------
   println( "\n---Begin to check result." );

   for( i = 0; i < dataType.length; i++ )
   {
      println( "---Check result for dataType[" + dataType[i] + "---" + numType[i] + "],  i=" + i + "." );

      var actLen = findRecsArray[i].length;
      var expLen = 1;
      if( actLen !== expLen )   //return size after find by type
      {
         throw buildException( "checkResult", null, "[compare number and key-b]",
            "[recsNum:" + expLen + "]",
            "[recsNum:" + actLen + "]" );
      }

      if( i < 5 )
      {
         var actB = findRecsArray[i][0]["b"];
         var expB = rawData[i][dataType[i]];
      }
      else if( i === 5 )   //type: subObj, return "subObj" 
      {
         var actB = JSON.stringify( findRecsArray[i] );
         var expB = '[{"a":5,"b":{"0":{"c":"test"}}}]';
      }
      else if( i === 6 )   //type: subObj, return "subObj" 
      {
         var actB = JSON.stringify( findRecsArray[i] );
         var expB = '[{"a":6,"b":[{"c":"test"}]}]';
      }
      else if( i > 6 && i < dataType.length - 2 )  //i=6, bug: 1745
      {
         var actB = findRecsArray[i][0]["b"].toString();
         var expB = rawData[i][dataType[i]].toString();
      }

      if( actB !== expB ) 
      {
         throw buildException( "checkResult", null, "[compare number and key-b]",
            "[exp: " + expB + "]",
            "[act: " + actB + "]" );
      }
   }
}