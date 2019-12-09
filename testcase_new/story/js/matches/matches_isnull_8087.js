/************************************************************************
*@Description:   seqDB-8087:使用$isnull:1查询，目标字段存在且为null，走索引查询
                 seqDB-8089:使用$isnull:1查询，目标字段存在且不为null，走索引查询
*@Author:  2016/5/20  xiaoni huang
************************************************************************/
main();

function main ()
{
   try
   {
      var clName = COMMCLNAME + "_matches8087";
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

   cl.createIndex( indexName, { b: 1 } );
}

function insertRecs ( cl )
{
   println( "\n---Begin to insert records." );

   cl.insert( [{ a: 0 },
   { a: 1, b: null },
   { a: 2, b: "" }] );
}

function findRecs ( cl )
{
   println( "\n---Begin to find records." );

   var rc = cl.find( { b: { $isnull: 1 } } ).sort( { a: 1 } );

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

   var findRtn = new Array();
   while( tmpRecs = rc.next() ) 
   {
      findRtn.push( tmpRecs.toObj() );
   }
   //compare number
   var expLen = 2;
   if( findRtn.length !== expLen )
   {
      throw buildException( "checkResult", null, "[compare number]",
         "[recsNum:" + expLen + "]",
         "[recsNum:" + findRtn.length + "]" );
   }
   //compare records
   if( findRtn[0]["b"] !== undefined || findRtn[1]["b"] !== null )
   {
      throw buildException( "checkResult", null, "[compare records]",
         "[b:" + undefined + ", b:" + null + "]",
         "[b:" + findRtn[0]["b"] + ", b:" + findRtn[1]["b"] + "]" );
   }
}