/************************************************************************
*@Description:   seqDB-8032:使用$elemMatch查询，目标字段为对象，不走索引查询 
*@Author:  2016/5/23  xiaoni huang
************************************************************************/
main();

function main ()
{
   try
   {
      var clName = COMMCLNAME + "_matches8032";
      var cl = readyCL( clName );

      var rawData = [{ a: 0, b: {} },
      { a: 1, b: { c: "test" } },
      { a: 2, b: { c: "test", d: "" } }];
      insertRecs( cl, rawData );

      var cond = { b: { $elemMatch: { c: "test", d: "" } } };
      var findRecsArray = findRecs( cl, cond );
      checkResult( findRecsArray, rawData );

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

function checkResult ( findRecsArray, rawData )
{
   println( "\n---Begin to check result." );

   //compare records number after find
   var expLen = 1;
   var actLen = findRecsArray.length;
   if( actLen !== expLen )
   {
      throw buildException( "checkResult", null, "[compare number]",
         "[recsNum:" + expLen + "]",
         "[recsNum:" + actLen + "]" );
   }

   //compare records
   var actB = findRecsArray[0]["b"].toString();
   var expB = rawData[2]["b"].toString();
   if( actB !== expB )
   {
      throw buildException( "checkResult", null, "[compare records]",
         '["b": ' + expB + ']',
         '["b": ' + actB + ']' );
   }
}