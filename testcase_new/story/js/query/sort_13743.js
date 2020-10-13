/******************************************************************************
@Description : 1. hash-cl sort [seqDB-13743]
@Modify list :
               2015-01-16 pusheng Ding  Init
               2020-08-17 Zixian Yan    Modify
******************************************************************************/
testConf.csName = COMMCSNAME + "_13743";
testConf.clName = COMMCLNAME + "_13743";
testConf.clOpt = { ShardingKey: { a: 1 }, ShardingType: 'hash', ReplSize: 0 };
testConf.useSrcGroup = true;
testConf.useDstGroup = true;
testConf.skipStandAlone = true;
testConf.skipOneDuplicatePerGroup = true;

main( test );

function test ( testPara )
{
   var indexName = "index_13743";
   var rownums = 10000;
   var hashCL = testPara.testCL;
   var sourceGroup = testPara.srcGroupName;
   var tarGroupList = testPara.dstGroupNames;

   //split ({Partition:1024} {Partition:2048}) {Partition:3072})
   var tarGroupIndex = -1;
   var stepPar = 1024;
   var partId = 3;
   var lowPar = 0;
   var highPar = 0;
   for( var i = 0; i < partId; i++ )
   {
      tarGroupIndex++;
      if( tarGroupIndex == tarGroupList.length )
      {
         tarGroupIndex = 0;
      }
      lowPar = i * stepPar;
      highPar = ( i + 1 ) * stepPar;
      hashCL.split( sourceGroup, tarGroupList[tarGroupIndex], { Partition: lowPar }, { Partition: highPar } );
   }

   var data = [];
   for( var i = 0; i < rownums; i++ )
   {
      data.push( { a: rownums - i, b: i, c: "abcdefghijkl" + i } );
   }
   hashCL.insert( data );

   //query1 - select a,b,c from foo.bar order by a descending
   var sel = hashCL.find( null, { a: 0, b: 'b', c: 'c' } ).sort( { a: -1 } );
   //expected result {a:rownums,...} {a:rownums-1,...} ... {a:1,...}
   checkRec( sel, data );

   hashCL.createIndex( indexName, { b: 1 } );

   //query2 - select b from cs.cl order by b
   var sel = hashCL.find( null, { b: 'b' } ).sort( { b: 1 } ).hint( { "": indexName } );
   //expected result {b:0} {b:1} ... {b:rownums-1}
   var i = 0;
   while( sel.next() )
   {
      var ret = sel.current();
      if( ret.toObj()['b'] != i )
      {
         throw new Error ( "\nFailed to run index query, check rc : b = " + ret.toObj()['b'] + " at " + i + " th position.");
      }
      i++;
   }
   if( i !== rownums )
   {
      throw new Error ( "\nReturned record number is : " + i);
   }
}
