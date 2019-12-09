/************************************************************************
*@Description:     seqDB-8100:使用$regex查询，不走索引查询
*@Author:  2016/5/23  xiaoni huang
************************************************************************/
main();

function main ()
{
   try
   {
      var clName = COMMCLNAME + "_matches8100";
      var cl = readyCL( clName );

      var rawData = insertRecs( cl, rawData );

      var findRecsArray = findRecs( cl );
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

   var rawData = [{ a: 0, str: "sequoiadb@163.com" },
   { a: 1, str: "18826411857" },
   { a: 2, str: " " }];
   for( i = 0; i < rawData.length; i++ )
   {
      cl.insert( rawData[i] )
   }
   return rawData;
}

function findRecs ( cl )
{
   println( "\n---Begin to find records." );

   var cond1 = { str: { $regex: '^[_a-z0-9]+@([_a-z0-9]+\.)+[_a-z0-9]{2,3}$', $options: 'i' } };
   var cond2 = { str: { $regex: '^[1][8][0-9]{9}$', $options: 'i' } };
   var cond3 = { str: { $regex: '^[ ]+$', $options: 'i' } };
   var condArray = [cond1, cond2, cond3];

   var findRecsArray = [];
   for( i = 0; i < condArray.length; i++ )
   {
      var rc = cl.find( condArray[i] ).sort( { a: 1 } );
      var tmpArray = [];
      while( tmpRecs = rc.next() )
      {
         tmpArray.push( tmpRecs.toObj() );
      }
      findRecsArray.push( tmpArray );
   }
   //println(JSON.stringify(findRecsArray));
   return findRecsArray;
}

function checkResult ( findRecsArray, rawData )
{
   println( "\n---Begin to check result." );

   //total results
   var expLen = 3;
   var actLen = findRecsArray.length;
   if( actLen !== expLen )
   {
      throw buildException( "checkResult", null, "[compare number]",
         "[recsNum:" + expLen + "]",
         "[recsNum:" + actLen + "]" );
   }

   //compare resulst for each find
   for( i = 0; i < findRecsArray.length; i++ )
   {
      //compare number
      var expLen = 1;
      var actLen = findRecsArray[i].length;
      if( actLen !== expLen )
      {
         throw buildException( "checkResult", null, "[compare number]",
            "[recsNum:" + expLen + "]",
            "[recsNum:" + actLen + "]" );
      }
      //compare records
      var actB = findRecsArray[i][0]["str"];
      var expB = rawData[i]["str"];
      if( actB !== expB )
      {
         throw buildException( "checkResult", null, "[compare records]",
            '["str": ' + expB + ']',
            '["str": ' + actB + ']' );
      }
   }
}