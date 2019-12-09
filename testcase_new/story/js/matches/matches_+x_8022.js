/************************************************************************
*@Description:   seqDB-8022:使用$+标识符查询，目标字段为非数组，不走索引查询 
*@Author:  2016/5/23  xiaoni huang
************************************************************************/
main();

function main ()
{
   try
   {
      var clName = COMMCLNAME + "_matches8022";
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

   cl.insert( { a: "test" } )
}

function findRecs ( cl )
{
   println( "\n---Begin to find records." );

   var rc = cl.find( { "a.$1": "test" } ).sort( { a: 1 } );
   return rc;
}

function checkResult ( rc )
{
   println( "\n---Begin to check result." );

   var findRecsArray = [];
   while( tmpRecs = rc.next() )
   {
      findRecsArray.push( tmpRecs.toObj() );
   }

   var expLen = 0;
   var actLen = findRecsArray.length;
   if( actLen !== expLen )
   {
      throw buildException( "checkResult", null, "[compare number]",
         "[recsNum:" + expLen + "]",
         "[recsNum:" + actLen + "]" );
   }
}