/************************************************************************
*@Description:   seqDB-8031:使用$all查询，给定值为空（如{$all:[]}）  
                     dataType: array
*@Author:  2016/5/20  xiaoni huang
************************************************************************/
main();

function main ()
{
   try
   {
      var clName = COMMCLNAME + "_matches8031";
      var cl = readyCL( clName );

      var rawData = [{ b: 2147483648 }, { c: 1.7E+308 }, { d: "test" }];
      insertRecs( cl, rawData );

      var rc1 = findRecs( cl, { a: { $all: [] } } );
      var rc2 = findRecs( cl, { a: { $all: [{ b: 2147483648 }, { d: "test" }] } } );
      var rc3 = findRecs( cl, { a: { $all: [{ b: 2 }] } } );

      checkResult( rc1, rc2, rc3, rawData );

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

function checkResult ( rc1, rc2, rc3, rawData )
{
   //-----------------------check result for $all[]---------------------
   println( "\n---Begin to check result for find by $all[]." );

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
   if( findRtn[0]["a"][0]["b"] !== rawData[0]["b"] ||
      findRtn[0]["a"][1]["c"] !== rawData[1]["c"] )
   {
      println( "---The real results after the find by matches[$all]: \n" + JSON.stringify( findRtn ) );
      throw buildException( "checkResult", null, "[compare records]",
         "[{b:" + rawData[0]["b"]
         + "}, {c:" + rawData[1]["c"] + "}]",
         "[{b:" + findRtn[0]["a"][0]["b"]
         + "}, {c:" + findRtn[0]["a"][1]["c"] + "}]" );
   }

   //-----------------------check result for $all[]---------------------
   println( "\n---Begin to check result for find by $all[{b:2147483648}, {d:'test'}]." );

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
   if( findRtn[0]["a"][0]["b"] !== rawData[0]["b"] ||
      findRtn[0]["a"][1]["c"] !== rawData[1]["c"] )
   {
      println( "---The real results after the find by matches[$all]: \n" + JSON.stringify( findRtn ) );
      throw buildException( "checkResult", null, "[compare records]",
         "[{b:" + rawData[0]["b"]
         + "}, {c:" + rawData[1]["c"] + "}]",
         "[{b:" + findRtn[0]["a"][0]["b"]
         + "}, {c:" + findRtn[0]["a"][1]["c"] + "}]" );
   }

   //-----------------------check result for $all[]---------------------
   println( "\n---Begin to check result for find by $all[{b:2}]." );
   var findRtn = new Array();
   while( tmpRecs = rc3.next() )  //rc3
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
}