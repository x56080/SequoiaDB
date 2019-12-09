/************************************************************************
*@Description:   seqDB-8018:使用$+标识符查询，目标字段为嵌套数组，不走索引查询
*@Author:  2016/5/23  xiaoni huang
*@bug: -------------jira:1745
************************************************************************/
main();

function main ()
{
   try
   {
      var clName = COMMCLNAME + "_matches8019";
      var indexName = CHANGEDPREFIX + "_index";
      var cl = readyCL( clName );
      createIndex( cl, indexName );

      var rawData = [{ a: 0, b: [1, 2, [3, 4, 5]] },
      { a: 1, b: [1, 3, [4, 5]] },
      { a: 2, b: [4, 2, 1] }];
      insertRecs( cl, rawData );

      var cond = { "b.2.$1": 5 };
      var findRecsArray = findRecs( cl, cond );
      //checkResult( cl, findRecsArray, rawData, indexName );

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

   cl.createIndex( indexName, { "b.2": 1 } );
}

function insertRecs ( cl, rawData )
{
   println( "\n---Begin to insert records." );
   for( i = 0; i < rawData.length; i++ )
   {
      cl.insert( rawData[i] )
   }
}

function findRecs ( cl, cond )
{
   println( "\n---Begin to find records." );

   var rc = cl.find( cond ).sort( { a: 1 } );
   var findRecsArray = [];
   while( tmpRecs = rc.next() )
   {
      findRecsArray.push( tmpRecs.toObj() );
   }
   //println(JSON.stringify(findRecsArray));
   return findRecsArray;
}

function checkResult ( cl, findRecsArray, rawData, indexName )
{
   //-------------------check index----------------------------
   println( "\n---Begin to check index." );

   //compare scanType
   var idx = cl.find( { "b.2.$1": 5 } ).explain().current().toObj();
   if( idx["ScanType"] !== "ixscan" || idx["IndexName"] !== indexName )
   {
      throw buildException( "checkResult", null, "[compare index]",
         "[ScanType:ixscan,IndexName:" + indexName + "]",
         "[ScanType:" + idx["ScanType"] + ",IndexName:" + idx["IndexName"] + "]" );
   }

   //-------------------check records----------------------------
   println( "\n---Begin to check result." );

   for( i = 0; i < rawData.length - 1; i++ )
   {
      //compare records number after find
      var expLen = 2;
      var actLen = findRecsArray.length;
      if( actLen !== expLen )
      {
         throw buildException( "checkResult", null, "[compare number]",
            "[recsNum:" + expLen + "]",
            "[recsNum:" + actLen + "]" );
      }

      //compare records
      var actB = findRecsArray[i]["b"].toString();
      var expB = rawData[i]["b"].toString();
      if( actB !== expB )
      {
         throw buildException( "checkResult", null, "[compare records]",
            '["b": ' + expB + ']',
            '["b": ' + actB + ']' );
      }
   }
}