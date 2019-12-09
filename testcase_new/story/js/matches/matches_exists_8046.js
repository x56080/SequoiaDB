/************************************************************************
*@Description:   seqDB-8046:使用$exists:1查询
*@Author:  2016/5/20  xiaoni huang
************************************************************************/
main();

function main ()
{
   try
   {
      var clName = COMMCLNAME + "_matches8046";
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

   var rc = cl.find( { b: { $exists: 1 } } ).sort( { a: 1 } );

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
   var expLen = 2;
   if( findRtn.length !== expLen )
   {
      throw buildException( "checkResult", null, "[compare number]",
         "[recsNum:" + expLen + "]",
         "[recsNum:" + findRtn.length + "]" );
   }
   //compare records
   if( findRtn[0]["b"] !== null || findRtn[1]["b"] !== "" )
   {
      throw buildException( "checkResult", null, "[compare records]",
         "[b:" + null + ", b:" + "" + "]",
         "[b:" + findRtn[0]["b"] + ", b:" + findRtn[1]["b"] + "]" );
   }
}