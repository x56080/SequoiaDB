/************************************************************************
*@Description:  seqDB-8059:使用$mod查询，除数为0，余数取0 
*@Author:  2016/5/19  xiaoni huang
************************************************************************/
main();

function main ()
{
   try
   {
      var clName = COMMCLNAME + "_matches8059";
      var cl = readyCL( clName );

      insertRecs( cl );
      var rc = findRecs( cl, 2, 0 );  //[div, rem]---[2,0]
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
   cl.insert( [{ a: 0 }] );
}

function findRecs ( cl, div, rem )
{
   println( "\n---Begin to find records by matches[$mod]." );

   var rc = cl.find( { a: { $mod: [div, rem] } } );

   return rc;
}

function checkResult ( rc )
{
   println( "\n---Begin to check result for [0 % 2 = 0]." );

   var findRtn = new Array();
   while( tmpRecs = rc.next() )
   {
      findRtn.push( tmpRecs.toObj() );
   }
   //compare records
   if( findRtn.length !== 1 || findRtn[0]["a"] !== 0 )
   {
      throw buildException( "checkResult", null, "[compare records]",
         "[findRtnLen:" + 1 + ", a:" + 0 + "]",
         "[findRtnLen:" + findRtn.length + ", a:" + findRtn[0]["a"] + "]" );
   }
}