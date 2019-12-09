/************************************************************************
*@Description:   seqDB-8034:使用$elemMatch查询，目标字段为非对象（如string）  
*@Author:  2016/5/23  xiaoni huang
************************************************************************/
main();

function main ()
{
   try
   {
      var clName = COMMCLNAME + "_matches8034";
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

   cl.insert( { a: [{ b: "test" }] } );
}

function findRecs ( cl )
{
   println( "\n---Begin to find records." );

   var rc = cl.find( { a: { $elemMatch: { "": "" } } } ).sort( { a: 1 } );
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
   var expLen = 0;
   var actLen = findRecsArray.length;
   if( actLen !== expLen )
   {
      throw buildException( "checkResult", null, "[compare number]",
         "[recsNum:" + expLen + "]",
         "[recsNum:" + actLen + "]" );
   }
}