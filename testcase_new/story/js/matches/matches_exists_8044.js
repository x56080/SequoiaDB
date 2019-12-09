/************************************************************************
*@Description:   seqDB-8044:使用$exists:0查询，不走索引查询 
*@Author:  2016/5/20  xiaoni huang
************************************************************************/
main();

function main ()
{
   try
   {
      var clName = COMMCLNAME + "_matches8044";
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

   var rc = cl.find( { b: { $exists: 0 } } ).sort( { a: 1 } );

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
   var expLen = 1;
   if( findRtn.length !== expLen )
   {
      throw buildException( "checkResult", null, "[compare number]",
         "[recsNum:" + expLen + "]",
         "[recsNum:" + findRtn.length + "]" );
   }
   //compare records
   if( findRtn[0]["b"] !== undefined )
   {
      throw buildException( "checkResult", null, "[compare records]",
         "[b:" + undefined + "]",
         "[b:" + findRtn[0]["b"] + "]" );
   }
}