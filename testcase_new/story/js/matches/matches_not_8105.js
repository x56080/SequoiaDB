/************************************************************************
*@Description:      seqDB-8105:使用$not查询，value取1个值
                    seqDB-8108:使用$not查询，给定值为空（如{$not:[]}）
*@Author:  2016/5/25  xiaoni huang
************************************************************************/
main();

function main ()
{
   try
   {
      var clName = COMMCLNAME + "_matches8105";
      var cl = readyCL( clName );

      insertRecs( cl );
      var rc1 = findRecs( cl, { $not: [{ a: 1 }] } );
      var rc2 = findRecs( cl, { $not: [] } );
      checkResult( rc1, rc2 );

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
   { a: 1, b: null }] );
}

function findRecs ( cl, cond )
{
   println( "\n---Begin to find records." );

   var rc = cl.find( cond ).sort( { a: 1 } );

   return rc;
}

function checkResult ( rc1, rc2 )
{
   //---------------------------------check results for find[$not:[{a:1}]]-----------------------------
   println( "\n---Begin to check result for find[ $not:[{a:1}] ]." );

   var findRtn = new Array();
   while( tmpRecs = rc1.next() )  //rc1
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

   //compare records
   if( findRtn[0]["a"] !== 0 )
   {
      throw buildException( "checkResult", null, "[compare records]",
         "[a:" + 0 + "]",
         "[a:" + findRtn[0]["a"] + "]" );
   }

   //---------------------------------check results for find[$not:[]]-----------------------------
   println( "\n---Begin to check result for find[ $not:[] ]." );

   var findRtn = new Array();
   while( tmpRecs = rc2.next() )  //rc2
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
   if( findRtn[0]["a"] !== 0 || findRtn[1]["a"] !== 1 )
   {
      throw buildException( "checkResult", null, "[compare records]",
         "[a:" + 0 + ", a:" + 1 + "]",
         "[a:" + findRtn[0]["a"] + ", a:" + findRtn[1]["a"] + "]" );
   }
}