/************************************************************************
*@Description:    seqDB-8103:相同字段组合测试
*@Author:  2016/5/25  xiaoni huang
************************************************************************/
main();

function main ()
{
   try
   {
      var clName = COMMCLNAME + "_matches8103";
      var cl = readyCL( clName );

      insertRecs( cl );

      var findRecsArray = findRecs( cl );
      checkResult( findRecsArray );

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

   cl.insert( [{ a: 0 }, { a: 1 }] );
}

function findRecs ( cl )
{
   println( "\n---Begin to find records." );

   var rc = cl.find( { a: { $lt: 0 }, a: { $et: 1 } } );
   var findRecsArray = [];
   while( tmpRecs = rc.next() )
   {
      findRecsArray.push( tmpRecs.toObj() );
   }
   //println(JSON.stringify(findRecsArray));
   return findRecsArray;
}

function checkResult ( findRecsArray )
{
   println( "\n---Begin to check result." );

   var expLen = 1;
   if( findRecsArray.length !== expLen )   //return size after find by type
   {
      throw buildException( "checkResult", null, "[compare number]",
         "[recsNum:" + expLen + "]",
         "[recsNum:" + findRecsArray.length + "]" );
   }
   //println(JSON.stringify(findRecsArray));
   var actA = findRecsArray[0]["a"];
   var expA = 1;
   if( actA !== expA )
   {
      throw buildException( "checkResult", null, "[compare records]",
         '["expA": ' + expA + ']',
         '["actA": ' + actA + ']' );
   }
}