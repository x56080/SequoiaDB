/******************************************************************************
@Description : 1. sort: a[1,2,3] sort
@Modify list :
               2015-01-15 pusheng Ding  Init
               2020-08-14 Zixian YAn    Modify
******************************************************************************/
testConf.clName = COMMCLNAME + "_13739";
main( test );
function test ( testPara )
{
   var indexName = "index_13739";
   var rownums = 1000;

   var cl = testPara.testCL;

   //insert data
   var data = []
   var reverseData = [];
   for( var i = 0; i < rownums; i++ )
   {
      data.push( { a: i, b: [ i + 1, i + 2, i + 3 ] } );
      var j = rownums - i;
      reverseData.push( { a: j - 1 , b: [ j, j + 1, j + 2 ] } );
   }
   cl.insert( reverseData );

   //query1 - select a,b from cl order by b without index
   var sel = cl.find( ).sort( { b: 1 } );
   checkRec( sel, data );
   println( "'select a,b from foo.bar order by b' finished!" );

   //create index
   cl.createIndex( indexName, { b: 1 } );
   println( "create indexes finished!" );

   //query2 - select a,b from cl order by b with index
   var sel = cl.find( ).sort( { b: 1 } ).hint( { "": indexName } );
   checkRec( sel, data );
   println( "'select a,b from foo.bar order by b' with index finished!" );
}
