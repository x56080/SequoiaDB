/************************************************************************
*@Description:    seqDB-10237 : 使用$field查询，并按_id字段排序
*@Author:  2016/10/18  xiaoni huang
************************************************************************/
main();

function main ()
{
   try
   {
      var clName = COMMCLNAME + "_matches10237";
      var cl = readyCL( clName );

      var rawData = insertRecs( cl );

      var findRecsArray = findRecs( cl );
      checkResult( findRecsArray );

      //cleanCL( clName );
   }
   catch( e )
   {
      throw e;
   }
}

function insertRecs ( cl )
{
   println( "\n---Begin to insert records." );

   cl.insert( { a: 1, b: 1 } );
   cl.insert( { a: 2, b: 2 } );
   cl.insert( { a: 1, b: 2 } );
}

function findRecs ( cl )
{
   println( "\n---Begin to find records." );

   var rc = cl.find( { a: { $field: "b" } } ).sort( { _id: 1 } )

   var findRecsArray = [];
   while( tmpRecs = rc.next() )
   {
      findRecsArray.push( tmpRecs.toObj() );
   }
   return findRecsArray;
}

function checkResult ( findRecsArray )
{
   println( "\n---Begin to check result." );

   var expLen = 2;
   if( findRecsArray.length !== expLen )   //return size after find by type
   {
      throw buildException( "checkResult", null, "[compare number]",
         "[recsNum:" + expLen + "]",
         "[recsNum:" + findRecsArray.length + "]" );
   }
   //println(JSON.stringify(findRecsArray));
   var actA1 = findRecsArray[0]["a"];
   var actB1 = findRecsArray[0]["b"];
   var actA2 = findRecsArray[1]["a"];
   var actB2 = findRecsArray[1]["b"];
   var actRecs = '{a:' + actA1 + ', b:' + actB1 + '}, {a:' + actA2 + ', b:' + actB2 + '}';
   var expRecs = '{a:1, b:1}, {a:2, b:2}';
   if( actRecs !== expRecs )
   {
      throw buildException( "checkResult", null, "[compare records]",
         '["expRecs": ' + expRecs + '}]',
         '["actRecs": ' + actRecs + '}]' );
   }
}