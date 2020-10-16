/************************************************************************
*@Description:   seqDB-8094:使用$size查询，目标字段为嵌套数组
                 seqDB-8096:使用$size查询，目标字段为数组且数组元素为嵌套对象 
*@Author:  2016/5/23  xiaoni huang
************************************************************************/
main();

function main ()
{
   try
   {
      var clName = COMMCLNAME + "_matches8094";
      var cl = readyCL( clName );

      var rawData = [{ a: 0, b: [] },
      { a: 1, b: [1] },
      { a: 2, b: [2, { c: "" }] },
      { a: 3, b: [3, null, ""] },
      {
         a: 4, b: [0,
            [{ c: null }],
            [{
               d: [{ e: "" },
               { f: [] }]
            }],
            { g: { h: { i: [] } } }]
      }];
      insertRecs( cl, rawData );

      var sizeNum = [-1, 0, 1, 2, 3, 4];
      var findRecsArray = findRecs( cl, sizeNum );
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

function findRecs ( cl, sizeNum )
{
   println( "\n---Begin to find records." );

   var findRecsArray = [];
   for( i = 0; i < sizeNum.length; i++ )
   {
      println( "---Find by matches[$size:" + sizeNum[i] + "]." );

      var rc = cl.find( { b: { $size: 1, $et: sizeNum[i] } } ).sort( { a: 1 } );
      var tmpArray = [];
      while( tmpRecs = rc.next() )
      {
         tmpArray.push( tmpRecs.toObj() );
      }
      //println(JSON.stringify(tmpArray));
      findRecsArray.push( tmpArray );
   }
   return findRecsArray;
}

function checkResult ( findRecsArray, rawData )
{
   println( "\n---Begin to check result." );

   for( i = 0; i < findRecsArray.length; i++ )
   {
      //compare records number after find
      if( i == 0 )
      {
         var expLen = 0;
      }
      else if( i > 0 )
      {
         var expLen = 1;
      }
      var actLen = findRecsArray[i].length;
      if( actLen !== expLen )
      {
         throw buildException( "checkResult", null, "[compare number]",
            "[recsNum:" + expLen + "]",
            "[recsNum:" + actLen + "]" );
      }

      //compare records
      if( i > 0 )   //i=0, results:[]
      {
         var actB = findRecsArray[i][0]["b"].toString();
         var expB = rawData[i - 1]["b"].toString();
         if( actB !== expB )
         {
            throw buildException( "checkResult", null, "[compare records]",
               '["b": ' + expB + ']',
               '["b": ' + actB + ']' );
         }
      }
   }
}