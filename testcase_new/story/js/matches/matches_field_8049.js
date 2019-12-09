/************************************************************************
*@Description:   seqDB-8049:使用$field查询，其他所有匹配符组合使用
*@Author:  2016/5/25  xiaoni huang
************************************************************************/
main();

function main ()
{
   try
   {
      var clName = COMMCLNAME + "_matches8049";
      var cl = readyCL( clName );

      var rawData = insertRecs( cl );

      var findRecsArray = findRecs( cl );
      checkResult( findRecsArray, rawData );

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

   var rawData = [{
      a: 0, a2: 1,
      int: -2147483648, int2: -2147483648,
      tmp3: null
   }];
   cl.insert( rawData );

   return rawData;
}

function findRecs ( cl )
{
   println( "\n---Begin to find records." );

   var tmpCond = [{ a: { $ne: { $field: "a2" } } },
   { int: { $et: { $field: "int2" } } },
   { null: { $isnull: { $field: "tmp3" } } }];
   var rmNum = parseInt( Math.random() * tmpCond.length );
   println( "---randomCond: " + JSON.stringify( tmpCond[rmNum] ) );
   var rc = cl.find( tmpCond[rmNum], { _id: { $include: 0 } } ).sort( { a: 1 } );
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

   var expLen = 1;
   if( findRecsArray.length !== expLen )   //return size after find by type
   {
      throw buildException( "checkResult", null, "[compare number]",
         "[recsNum:" + expLen + "]",
         "[recsNum:" + findRecsArray.length + "]" );
   }
   //println(JSON.stringify(findRecsArray));
   var actRecs = JSON.stringify( findRecsArray[0] );
   var extRecs = JSON.stringify( rawData[0] );
   if( actRecs !== extRecs )
   {
      throw buildException( "checkResult", null, "[compare records]",
         '["extRecs": ' + extRecs + ']',
         '["actRecs": ' + actRecs + ']' );
   }
}