/************************************************************************
*@Description:   seqDB-8023:使用$+标识符查询，目标字段为非数组，走索引查询 
*@Author:  2016/5/23  xiaoni huang
************************************************************************/
main();

function main ()
{
   try
   {
      var clName = COMMCLNAME + "_matches8023";
      var indexName = CHANGEDPREFIX + "_index";
      var cl = readyCL( clName );
      createIndex( cl, indexName );

      insertRecs( cl );

      var rc = findRecs( cl );
      checkResult( rc, indexName );

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

function insertRecs ( cl )
{
   println( "\n---Begin to insert records." );

   cl.insert( { a: "test" } )
}

function findRecs ( cl )
{
   println( "\n---Begin to find records." );

   var rc = cl.find( { "a.$1": "test" } ).sort( { a: 1 } );
   return rc;
}

function checkResult ( rc, indexName )
{
   //-------------------check index----------------------------
   println( "\n---Begin to check index." );

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

   var expLen = 0;
   var actLen = findRecsArray.length;
   if( actLen !== expLen )
   {
      throw buildException( "checkResult", null, "[compare number]",
         "[recsNum:" + expLen + "]",
         "[recsNum:" + actLen + "]" );
   }
}