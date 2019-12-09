/************************************************************************
*@Description:   seqDB-8075:使用$ne查询，目标字段为非数值型，不走索引查询
                     data type: null/string/bool/oid/regex/date/timestamp
*@Author:  2016/5/18  xiaoni huang
************************************************************************/
main();

function main ()
{
   try
   {
      var clName = COMMCLNAME + "_matches8075";

      var cl = readyCL( clName );

      var dataType = ["null", "string", "bool", "oid", "regex", "date", "timestamp"];
      var rawData = [{ null: null },
      { string: "hello world" },
      { bool: true },
      { oid: { "$oid": "123abcd00ef12358902300ef" } },
      { regex: { "$regex": "^rg", "$options": "i" } },
      { date: { "$date": "2038-01-18" } },
      { timestamp: { "$timestamp": "2038-01-18-23.59.59.999999" } }];
      insertRecs( cl, rawData, dataType );

      var findRecsArray = findRecs( cl, rawData, dataType );

      checkResult( findRecsArray, dataType );

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

   var j = 0;
   var findRecsArray = [];
   for( i = 0; i < rawData.length; i++ )
   {
      println( "---Find for dataType[" + dataType[i] + "]." );

      var rc = cl.find( { b: { $ne: rawData[i][dataType[i]] } } ).sort( { a: 1 } );
      var tmpArray = [];
      while( tmpRecs = rc.next() )
      {
         tmpArray.push( tmpRecs.toObj() );
      }
      findRecsArray.push( tmpArray );;
   }
   return findRecsArray;
}

function checkResult ( findRecsArray, dataType )
{
   println( "\n---Begin to check result." );

   for( i = 0; i < findRecsArray.length; i++ )
   {
      println( "---Check result for dataType[" + dataType[i] + "], i=" + i + "." );

      var expLen = 6;  //totalRecsCount:7, 6 after exec find["ne"]
      if( findRecsArray[i].length !== expLen )
      {
         throw buildException( "checkResult", null, "[compare number]",
            "[recsNum:" + expLen + "]",
            "[recsNum:" + findRecsArray[i].length + "]" );
      }

      for( j = 0; j < findRecsArray[i].length; j++ )
      {
         if( findRecsArray[i][j]["a"] === i )
         {
            println( "---The real results for dataType[" + dataType[i] + "]: \n" + JSON.stringify( findRecsArray[i] ) );
            throw buildException( "checkResult", null, "[compare records]",
               '[not contain {"a": ' + i + '}]',
               '[contain {"a": ' + findRecsArray[i][j]["a"] + '}]' );
         }
      }
   }
}