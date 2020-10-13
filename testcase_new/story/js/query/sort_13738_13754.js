/******************************************************************************
@Description : 1. sort: complex column sort
@Modify list :
               2015-01-15 pusheng Ding  Init
               2020-08-17 Zixian Yan
******************************************************************************/
testConf.clName = COMMCLNAME + "_13738_13754";

var rownums = 100;

main( test );
function test ( testPara )
{
   var cl = testPara.testCL;

   var data = [];
   var reverseData = [];
	for( var i = 0; i < rownums; i++ )
	{
      var j = rownums - i;
		data.push( { a: { A1: i, A2: j }, b: "abc" + i } );
      reverseData.push( { a: { A1: j -1 , A2: i + 1 }, b: "abc" + (j -1) } );
	}

   cl.insert( data );

   // sort_13754 --- Query 1
   var sel = cl.find().sort( { a: 1 } );
   checkRec(sel, data );
   var sel = cl.find().sort( { a: -1 } );
   checkRec( sel, reverseData );


   // sort_13738 --- Query 1
   var sel = cl.find().sort( { "a.A1": 1 } );
   checkRec(sel, data );
   var sel = cl.find().sort( { "a.A1": -1 } );
   checkRec( sel, reverseData );

   var indexName = "index_idx";
   cl.createIndex( indexName, { a: 1 } );

   // // sort_13754 --- Query 2
   var sel2 = cl.find().sort( { a: 1 } ).hint( { "": indexName } );
   checkRec( sel2, data );

   // sort_13738 --- Query 2
   var sel2 = cl.find().sort( { "a.A1": 1 } ).hint( { "": indexName } );
   checkRec( sel2, data )

}
