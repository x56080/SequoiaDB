/************************************************************************
*@Description:   seqDB-8086:使用$isnull:1查询，目标字段存在且为null，不走索引查询
                 seqDB-8088:使用$isnull:1查询，目标字段存在且不为null，不走索引查询
                 seqDB-8090:使用$isnull:1查询，目标字段不存在
*@Author:  2016/5/20  xiaoni huang
************************************************************************/
main();

function main ()
{
   try
   {
      var clName = COMMCLNAME + "_matches8086";
      var cl = readyCL( clName );

      insertRecs( cl );
      var rc = findRecs( cl );
      checkResult( rc );

      cleanCL( clName );
   }
   catch( e )
   {
      throw e;
   }
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

function checkResult ( rc )
{
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