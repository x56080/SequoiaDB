/************************************************************************
*@Description:   seqDB-8063:使用$in查询，不走索引查询 
                    cover all data type
*@Author:  2016/5/20  xiaoni huang
************************************************************************/
main();

function main ()
{
   try
   {
      var clName = COMMCLNAME + "_matches8063";

      var cl = readyCL( clName );

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
      { timestamp: { "$timestamp": "2038-01-18-23.59.59.999999" } },
      { tmp: 1 }];
      insertRecs( cl, rawData, dataType );

      var rc = findRecs( cl, rawData, dataType );

      checkResult( rc, rawData, dataType );

      cleanCL( clName );
   }
   catch( e )
   {
      throw e;
   }
}

function insertRecs ( cl, rawData, dataType )
{
   println( "\n---Begin to insert records." );

   for( i = 0; i < rawData.length; i++ )
   {
      cl.insert( { a: i, b: rawData[i][dataType[i]] } );
   }
}

function findRecs ( cl, rawData, dataType )
{
   println( "\n---Begin to find records." );

   var tmpValue = [];
   for( i = 0; i < dataType.length; i++ )
   {
      tmpValue.push( rawData[i][dataType[i]] );
   }
   var rc = cl.find( { b: { $in: tmpValue } } ).sort( { a: 1 } );

   return rc;
}

function checkResult ( rc, rawData, dataType )
{
   println( "\n---Begin to check result." );

   var findRecsArray = [];
   while( tmpRecs = rc.next() )
   {
      findRecsArray.push( tmpRecs.toObj() );
   }
   //println(JSON.stringify(findRecsArray));

   //compare number
   var expLen = 11;
   if( findRecsArray.length !== expLen )
   {
      throw buildException( "checkResult", null, "[compare number]",
         "[recsNum:" + expLen + "]",
         "[recsNum:" + findRecsArray.length + "]" );
   }

   //compare records
   for( i = 0; i < findRecsArray.length; i++ )
   {
      println( "---Check result for dataType[" + dataType[i] + "], i=" + i + "." );

      if( i < 5 )
      {
         var actB = findRecsArray[i]["b"];
         var expB = rawData[i][dataType[i]];
      }
      else
      {
         var expB = rawData[i][dataType[i]].toString();
         var actB = findRecsArray[i]["b"].toString();
      }

      if( actB !== expB )
      {
         throw buildException( "checkResult", null, "[compare records]",
            '["b": ' + expB + ']',
            '["b": ' + actB + ']' );
      }
   }
}