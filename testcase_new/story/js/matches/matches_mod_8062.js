/************************************************************************
*@Description:  seqDB-8062:使用$mod查询，除数为其他类型数据 
*@Author:  2016/5/20  xiaoni huang
************************************************************************/
main();

function main ()
{
   try
   {
      var clName = COMMCLNAME + "_matches8062";
      var cl = readyCL( clName );

      insertRecs( cl );
      var rc = findRecs( cl, 1, 0 );  //[div, rem]---[1,0]
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
   cl.insert( [{ a: "test" }] );
}

function findRecs ( cl, div, rem )
{
   println( "\n---Begin to find records by matches[$mod]." );

   var rc = cl.find( { a: { $mod: [div, rem] } } );

   return rc;
}

function checkResult ( rc )
{
   println( "\n---Begin to check result for ['test' % 1 = 0]." );

   var findRtn = new Array();
   while( tmpRecs = rc.next() )
   {
      findRtn.push( tmpRecs.toObj() );
   }
   //compare records
   if( findRtn.length !== 0 )
   {
      throw buildException( "checkResult", null, "[compare records]",
         "[findRtnLen:" + 0 + "]",
         "[findRtnLen:" + findRtn.length + "]" );
   }
}