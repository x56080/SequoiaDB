/************************************************************************
*@Description:      seqDB-8077:使用$or查询，value取1个值
                    seqDB-8080:使用$or查询，给定值为空（如{$or:[]}）
*@Author:  2016/5/24  xiaoni huang
************************************************************************/
main();

function main ()
{
   try
   {
      var clName = COMMCLNAME + "_matches8077";
      var cl = readyCL( clName );

      insertRecs( cl );
      var rc1 = findRecs( cl, { $or: [{ a: 0 }] } );
      var rc2 = findRecs( cl, { $or: [] } );
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
   //---------------------------------check results for find[$or:[{a:1}]]-----------------------------
   println( "\n---Begin to check result for find[ $or:[{a:1}] ]." );

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
         "[b:" + 0 + "]",
         "[b:" + findRtn[0]["a"] + "]" );
   }

   //---------------------------------check results for find[$or:[{a:1}]]-----------------------------
   println( "\n---Begin to check result for find[ $or:[] ]." );

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
         "[b:" + 0 + ", b:" + 1 + "]",
         "[b:" + findRtn[0]["a"] + ", b:" + findRtn[1]["a"] + "]" );
   }
}