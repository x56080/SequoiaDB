/************************************************************************
*@Description:   seqDB-8029:使用$all查询，走索引查询 
                    cover all data type
*@Author:  2016/5/21  xiaoni huang
************************************************************************/
main();

function main ()
{
   try
   {
      var clName = COMMCLNAME + "_matches8029";
      var indexName = CHANGEDPREFIX + "_index";
      var cl = readyCL( clName );
      createIndex( cl, indexName );

      //typeNum: 11
      var dataType = ["int", "double", "null", "string", "bool",
         "long", "oid", "regex", "binary", "date", "timestamp"];
      var rawData = [{ int: -2147483648 },
      { double: -1.7E+308 },
      { null: null },
      { string: "test" },
      { bool: true },
      { long: { "$numberLong": "-9223372036854775808" } },
      { oid: { "$oid": "123abcd00ef12358902300ef" } },
      { regex: { "$regex": "^rg", "$options": "i" } },
      { binary: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" } },
      { date: { "$date": "2038-01-18" } },
      { timestamp: { "$timestamp": "2038-01-18-23.59.59.999999" } }];
      insertRecs( cl, rawData, dataType );

      var rc = findRecs( cl, rawData, dataType );

      checkResult( rc, rawData, dataType, indexName );

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

   cl.createIndex( indexName, { a: 1 } );
}

function insertRecs ( cl, rawData, dataType )
{
   println( "\n---Begin to insert records." );

   var tmpValue = [];
   for( i = 0; i < rawData.length; i++ )
   {
      tmpValue.push( rawData[i][dataType[i]] );
   }
   cl.insert( { a: tmpValue } );
}

function findRecs ( cl, rawData, dataType )
{
   println( "\n---Begin to find records." );

   var tmpValue = [];
   for( i = 0; i < dataType.length; i++ )
   {
      tmpValue.push( rawData[i][dataType[i]] );
   }
   var rc = cl.find( { a: { $all: tmpValue } } ).sort( { a: 1 } );

   return rc;
}

function checkResult ( rc, rawData, dataType, indexName )
{
   //-------------------check index----------------------------
   println( "\n---Begin to check index." );

   //compare scanType
   var idx = rc.explain().current().toObj();
   if( idx["ScanType"] !== "ixscan" || idx["IndexName"] !== indexName )
   {
      throw buildException( "checkResult", null, "[compare index]",
         "[ScanType:ixscan,IndexName:" + indexName + "]",
         "[ScanType:" + idx["ScanType"] + ",IndexName:" + idx["IndexName"] + "]" );
   }

   //-------------------check records----------------------------
   println( "\n---Begin to check result." );

   var findRecsArray = [];
   while( tmpRecs = rc.next() )
   {
      findRecsArray.push( tmpRecs.toObj() );
   }
   //println(JSON.stringify(findRecsArray));

   //compare number
   var expLen = 1;
   if( findRecsArray.length !== expLen )
   {
      throw buildException( "checkResult", null, "[compare number]",
         "[recsNum:" + expLen + "]",
         "[recsNum:" + findRecsArray.length + "]" );
   }

   //compare records
   for( i = 0; i < rawData.length; i++ )
   {
      println( "---Check result for dataType[" + dataType[i] + "], i=" + i + "." );

      if( i < 5 )
      {
         var actA = findRecsArray[0]["a"][i];
         var expA = rawData[i][dataType[i]];
      }
      else
      {
         var actA = findRecsArray[0]["a"][i].toString();
         var expA = rawData[i][dataType[i]].toString();
      }

      if( actA !== expA )
      {
         throw buildException( "checkResult", null, "[compare records]",
            '["a": ' + expA + ']',
            '["a": ' + actA + ']' );
      }
   }
}
