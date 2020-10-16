/******************************************************************************
@Description : 1. range-cl sort  [seqDB-13742]
@Modify list :
               2015-01-16 pusheng Ding  Init
               2020-08-17 Zixian Yan    Modify
******************************************************************************/
testConf.csName = COMMCSNAME ;
testConf.clName = COMMCLNAME + "_13742";
testConf.clOpt = { ShardingKey: { a: 1 }, ShardingType: 'range', ReplSize: 0 };
testConf.useSrcGroup = true;
testConf.useDstGroup = true;
testConf.skipStandAlone = true;
testConf.skipOneDuplicatePerGroup = true;

main( test );

function test ( testPara )
{
   var rangeCL = testPara.testCL;
   var sourceGroup = testPara.srcGroupName;
   var indexName = "index_13742";
   var rownums = 10000;
   var dstGrouplist = testPara.dstGroupNames;

   var stepId = 5000;
   var partId = rownums / stepId;
   var lowId = 0;
   var highId = 0;
   for( var i = 0; i < partId; i++ )
   {
      lowId = ( i - 1 ) * stepId;
      highId = i * stepId;
      rangeCL.split( sourceGroup, dstGrouplist[i], { a: lowId }, { a: highId } );

      if ( (i+1) == dstGrouplist.length)
      {
         break;
      }
   }

   //insert data
   var records = [];
   for( var i = 0; i < rownums; i++ )
   {
      records.push( { a: rownums - i, b: i, c: "abcdefghijkl" + i } );
   }
   rangeCL.insert( records );

   //query1 - select a,b,c from cs.cl order by a descending
   var sel = rangeCL.find( null, { a: 0, b: 'b', c: 'c' } ).sort( { a: -1 } );
   checkRec( sel, records );

   rangeCL.createIndex( indexName, { b: 1 } );

   //query2 - select b from cs.cl order by b
   var sel = rangeCL.find( null, { b: 'b' } ).sort( { b: 1 } ).hint( { "": indexName } );
   var i = 0;
   while( sel.next() )
   {
      var ret = sel.current();
      if( ret.toObj()['b'] != i )
      {
         throw new Error ( "main() failed to run index query, check record in "+ (i+1) +"row, which doesn't equal  " + i );
      }
      i++;
   }
   if( i !== rownums )
   {
      throw new Error ("Returned record number is : " + i);
   }

}
