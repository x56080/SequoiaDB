/************************************************************************
*@Description:   seqDB-8066:使用$in查询，给定值为空（如{$in:[]}） 
                      seqDB-8063, value为1个
                     dataType: array
*@Author:  2016/5/20  xiaoni huang
************************************************************************/
main();

function main ()
{
   try
   {
      var clName = COMMCLNAME + "_matches8066";
      var cl = readyCL( clName );

      var rawData = [{ b: 2147483648 }, { c: 1.7E+308 }];
      insertRecs( cl, rawData );

      var rc1 = findRecs( cl, { a: { $in: [] } } );
      var rc2 = findRecs( cl, { a: { $in: [{ b: 2147483648 }] } } );

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

   cl.insert( { a: rawData } );
}

function findRecs ( cl, cond )
{
   println( "\n---Begin to find records by cond[" + JSON.stringify( cond ) + "]." );

   var rc = cl.find( cond );

   return rc;
}

function checkResult ( rc1, rc2, rawData )
{
   //-----------------------check result for $in[]---------------------
   println( "\n---Begin to check result for find by $in[]." );

   var findRtn = new Array();
   while( tmpRecs = rc1.next() )  //rc1
   {
      findRtn.push( tmpRecs.toObj() );
   }
   //compare number
   var expLen = 0;
   if( findRtn.length !== expLen )
   {
      throw buildException( "checkResult", null, "[compare number]",
         "[recsNum:" + expLen + "]",
         "[recsNum:" + findRtn.length + "]" );
   }

   //-----------------------check result for $in[]---------------------
   println( "\n---Begin to check result for find by $in[{b:2147483648}]." );

   var findRtn = new Array();
   while( tmpRecs = rc2.next() )  //rc2
   {
      findRtn.push( tmpRecs.toObj() );
   }
   //compare results
   var expLen = 1;
   if( findRtn.length !== expLen )
   {
      throw buildException( "checkResult", null, "[compare number]",
         "[recsNum:" + expLen + "]",
         "[recsNum:" + findRtn.length + "]" );
   }
   //compare records
   if( findRtn[0]["a"][0]["b"] !== rawData[0]["b"] ||
      findRtn[0]["a"][1]["c"] !== rawData[1]["c"] )
   {
      println( "---The real results after the find by matches[$in]: \n" + JSON.stringify( findRtn ) );
      throw buildException( "checkResult", null, "[compare records]",
         "[{b:" + rawData[0]["b"]
         + "}, {c:" + rawData[1]["c"] + "}]",
         "[{b:" + findRtn[0]["a"][0]["b"]
         + "}, {c:" + findRtn[0]["a"][1]["c"] + "}]" );
   }
}