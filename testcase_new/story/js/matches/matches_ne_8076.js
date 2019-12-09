/************************************************************************
*@Description:    seqDB-8076:使用$ne查询，目标字段为非数值型，走索引查询
                     data type: null/string/bool/oid/regex/date/timestamp 
                     index:{b:1}
*@Author:  2016/5/18  xiaoni huang
************************************************************************/
main();

function main ()
{
   try
   {
      var clName = COMMCLNAME + "_matches8076";
      var indexName = CHANGEDPREFIX + "_index";

      var cl = readyCL( clName );
      createIndex( cl, indexName );

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

      checkResult( cl, findRecsArray, dataType, indexName );

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

function checkResult ( cl, findRecsArray, dataType, indexName )
{
   //-------------------check index----------------------------
   println( "\n---Begin to check index." );

   //compare scanType
   var rc = cl.find( { b: { $ne: null } } ).sort( { a: 1 } ).hint( { '': '' } ).explain().current().toObj();
   if( rc["ScanType"] !== "ixscan" || rc["IndexName"] !== indexName )
   {
      throw buildException( "checkResult", null, "[compare index]",
         "[ScanType:ixscan,IndexName:" + indexName + "]",
         "[ScanType:" + rc["ScanType"] + ",IndexName:" + rc["IndexName"] + "]" );
   }

   println( "\n---Begin to check tblscan." );

   //compare scanType
   var rc = cl.find( { b: { $ne: null } } ).sort( { a: 1 } ).explain().current().toObj();
   if( rc["ScanType"] !== "tbscan" )
   {
      throw buildException( "checkResult", null, "[compare tbscan]",
         "[ScanType:tbscan]",
         "[ScanType:" + rc["ScanType"] + ",IndexName:" + rc["IndexName"] + "]" );
   }

   //-------------------check records----------------------------
   println( "\n---Begin to check results." );

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
