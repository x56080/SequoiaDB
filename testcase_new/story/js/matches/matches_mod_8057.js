/************************************************************************
*@Description:  seqDB-8057:使用$mod查询，除数和被除数符号相同 
                seqDB-8058:使用$mod查询，除数和被除数符号不同
                  除数(b) %  被除数(div)  = 余数(rem)
*@Author:  2016/5/20  xiaoni huang
************************************************************************/
main();

function main ()
{
   try
   {
      var clName = COMMCLNAME + "_matches8057";
      var cl = readyCL( clName );

      var rawData = [-3, -3, -3, -1, -4, 2, 5, 4, 4, -6];
      var div = [-5, 4, 2, 6, -3, 4, 3, -7, -3, -5];
      var rem = [-3, -3, -1, -1, -1, 2, 2, 4, 1, -1];
      insertRecs( cl, rawData );

      var findRecsArray = findRecs( cl, div, rem );
      checkResult( findRecsArray, rawData, div, rem );

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
      cl.insert( { a: i, b: rawData[i] } );
   }
}

function findRecs ( cl, div, rem )
{
   println( "\n---Begin to find records by matches[$mod]." );
   var findRecsArray = [];
   for( i = 0; i < div.length; i++ )
   {
      var rc = cl.find( { a: i, b: { $mod: [div[i], rem[i]] } } );
      var tmpRecsArray = new Array();
      while( tmpRecs = rc.next() )  //rc1
      {
         tmpRecsArray.push( tmpRecs.toObj() );
      }

      findRecsArray.push( tmpRecsArray );
   }

   return findRecsArray;
}

function checkResult ( findRecsArray, rawData, div, rem )
{
   for( i = 0; i < findRecsArray.length; i++ )
   {
      println( "\n---Begin to check result for [" + rawData[i] + " % " + div[i] + " = " + rem[i] + "]." );

      //compare number
      var expLen = 1;
      if( findRecsArray[i].length !== expLen )
      {
         println( "---The real results after the find by matches[$mod]: \n" + JSON.stringify( findRecsArray[i] ) );
         throw buildException( "checkResult", null, "[compare number]",
            "[recsNum:" + expLen + "]",
            "[recsNum:" + findRecsArray[i].length + "]" );
      }
      //compare records
      if( findRecsArray[i][0]["b"] !== rawData[i] )
      {
         println( "---The real results after the find by matches[$mod]: \n" + JSON.stringify( findRecsArray[i] ) );
         throw buildException( "checkResult", null, "[compare records]",
            "[b:" + rawData[i] + "]",
            "[b:" + findRecsArray[i][0]["b"] + "]" );
      }
   }
}