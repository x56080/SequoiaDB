/************************************************************************
*@Description:   seqDB-8053:使用$mod查询，除数/被除数/余数都为浮点数，不走索引查询
*@Author:  2016/5/19  xiaoni huang
************************************************************************/
main();

function main ()
{
   try
   {
      var clName = COMMCLNAME + "_matches8053";
      var cl = readyCL( clName );

      var rawData = [-2147483648.03, 2147483647.6];
      insertRecs( cl, rawData );
      var rc1 = findRecs( cl, -2147483647.6, -0.4300003051757812 );
      var rc2 = findRecs( cl, 2147483647.03, 0.5699999332427979 );
      checkResult( rc1, rc2, rawData );

      cleanCL( clName );
   }
   catch( e )
   {
      throw e;
   }
}

function insertRecs ( cl, rawData )
{
   println( "\n---Begin to insert records." );
   cl.insert( [{ a: rawData[0] }, { a: rawData[1] }] );
}

function findRecs ( cl, div, rem )
{
   println( "\n---Begin to find records by matches[$mod]." );

   var rc = cl.find( { a: { $mod: [div, rem] } } );

   return rc;
}

function checkResult ( rc1, rc2, rawData )
{
   //-----------------------check result for $mod[2,0]---------------------
   println( "\n---Begin to check result for find by $mod[2,0]." );

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
   if( findRtn[0]["a"] !== rawData[0] )
   {
      println( "---The real results after the find by matches[$mod]: \n" + JSON.stringify( findRtn ) );
      throw buildException( "checkResult", null, "[compare records]",
         "[a:" + rawData[0] + "]",
         "[a:" + findRtn[0]["a"] + "]" );
   }

   //-----------------------check result for $mod[2,1]---------------------
   println( "\n---Begin to check result for find by $mod[2,1]." );

   var findRtn = new Array();
   while( tmpRecs = rc2.next() )  //rc2
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
   if( findRtn[0]["a"] !== rawData[1] )
   {
      println( "---The real results after the find by matches[$mod]: \n" + JSON.stringify( findRtn ) );
      throw buildException( "checkResult", null, "[compare records]",
         "[a:" + rawData[1] + "]",
         "[a:" + findRtn[0]["a"] + "]" );
   }
}