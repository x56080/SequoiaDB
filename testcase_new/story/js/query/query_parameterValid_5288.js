/****************************************************
@description: seqDB-5288: skip，参数超过边界值
@author:
              2019-6-3 wuyan init
****************************************************/
main();

function main ()
{
   var clName = COMMCLNAME + "_query5288";
   var cl = readyCL( clName );

   var recordNum = 100;
   var expRecords = insertRecs( cl, recordNum );

   println( "---Begin to test skip < 0(skip = -1). " );
   var skipNum = -1;
   queryRecsAndCheckResult( cl, expRecords, skipNum );

   println( "---Begin to test skip > maxInt(skip = 2147483648). " );
   var skipNum = 2147483648;
   queryRecsAndCheckResult( cl, expRecords, skipNum );

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
   return docs;
}

function queryRecsAndCheckResult ( cl, expRecs, skipNum )
{
   var query = cl.find().skip( skipNum );
   checkRec( query, expRecs );
}