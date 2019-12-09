/****************************************************
@description: seqDB-5286:limit、skip，参数边界值校验
@author:
              2019-6-3 wuyan init
****************************************************/
main();

function main ()
{
   var clName = COMMCLNAME + "_query5286";
   var cl = readyCL( clName );

   var recordNum = 10000;
   insertRecs( cl, recordNum );
   queryRecsAndCheckResult( cl, recordNum );

   cleanCL( clName );

}

function insertRecs ( cl, recordNum )
{
   println( "\n---Begin to insert records." );

   var docs = [];
   for( i = 0; i < recordNum; i++ )
   {
      var objs = { "no": i, "str": "test" + i };
      docs.push( objs );
   }
   cl.insert( docs );
}

function queryRecsAndCheckResult ( cl, recordNum )
{
   println( "\n---Begin to test a: limit=0." );
   var query1 = cl.find().limit( 0 ).toArray();
   var expReturnEmpty = 0;
   if( query1.length !== expReturnEmpty )
   {
      throw buildException( "checkResult", null, "check return recordNum",
         "expQueryNum1:" + expReturnEmpty, "retrun count:" + query1.length
         + "\n record:" + JSON.stringify( query1 ) );
   }

   println( "\n---Begin to test a: limit=1,skip is max." );
   var query2 = cl.find().limit( 1 ).skip( recordNum ).toArray();
   if( query2.length !== expReturnEmpty )
   {
      throw buildException( "checkResult", null, "check return recordNum",
         "expQueryNum2:" + expReturnEmpty, "retrun count:" + query2.length
         + "\n record:" + JSON.stringify( query2 ) );
   }

   println( "\n---Begin to test b: limit is max,skip=0." );
   var query3 = cl.find().limit( recordNum ).skip( 0 ).toArray();
   if( query3.length !== recordNum )
   {
      throw buildException( "checkResult", null, "check return recordNum",
         "expQueryNum3:" + recordNum, "retrun count:" + query3.length
         + "\n record:" + JSON.stringify( query3 ) );
   }

   println( "\n---Begin to test c: skip is max." );
   var query4 = cl.find().skip( recordNum ).toArray();
   if( query4.length !== expReturnEmpty )
   {
      throw buildException( "checkResult", null, "check return recordNum",
         "expQueryNum4:" + expReturnEmpty, "retrun count:" + query4.length
         + "\n record:" + JSON.stringify( query4 ) );
   }

}