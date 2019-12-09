/************************************************************************
*@Description:   seqDB-8048:使用$field查询，t1字段和t2字段为不同数据类型
                    cover all data type
*@Author:  2016/5/25  xiaoni huang
************************************************************************/
main();

function main ()
{
   try
   {
      var clName = COMMCLNAME + "_matches8048";
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
      a: 0,
      int: 2147483647,
      double: 2147483647.00,
      long: { "$numberLong": "2147483647" },
      decimal: { "$decimal": "2147483647.00" }
   }];
   cl.insert( rawData );

   return rawData;
}

function findRecs ( cl )
{
   println( "\n---Begin to find records." );
   dataType = ["int", "double", "long", "decimal"];
   var rmNum1 = parseInt( Math.random() * dataType.length );
   var rmNum2 = parseInt( Math.random() * dataType.length );

   //field variable
   var cond = new Object();
   var field1 = dataType[rmNum1];
   cond[field1] = { $field: dataType[rmNum2] };
   //condition of find
   println( "---cond: " + JSON.stringify( cond ) );
   //find
   var rc = cl.find( cond, { _id: { $include: 0 } } ).sort( { a: 1 } );

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
   var extRecs = '{"a":0,"int":2147483647,"double":2147483647,"long":2147483647,"decimal":{"$decimal":"2147483647.00"}}';
   if( actRecs !== extRecs )
   {
      throw buildException( "checkResult", null, "[compare records]",
         '["extRecs": ' + extRecs + ']',
         '["actRecs": ' + actRecs + ']' );
   }
}