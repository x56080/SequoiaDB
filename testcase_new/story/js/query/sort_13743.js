/******************************************************************************
@Description : 1. hash-cl sort
@Modify list :
               2015-01-16 pusheng Ding  Init
******************************************************************************/
main();
function main ()
{
   if( commIsStandalone( db ) )
   {
      println( " Deploy mode is standalone!" );
      return;
   }
   //get ReplicaGroups
   var grouplist = Array();
   var cur = db.listReplicaGroups();
   while( cur.next() )
   {
      if( cur.current().toObj()['GroupID'] >= DATA_GROUP_ID_BEGIN )
      {
         grouplist.push( cur.current().toObj()['GroupName'] );
      }
   }
   var group_num = grouplist.length;
   if( group_num == 1 )
   {
      println( "only one ReplicaGroup:" + grouplist + " Skip the testcase" );
      return;
   }
   println( "ReplicaGroups: " + grouplist );

   var csName = COMMCSNAME;
   var clName = "cl13743";
   var indexName = "index13743";
   var rownums = 10000;
   commDropCL( db, csName, clName, true, true, "drop cl in the beginning" );

   var options = { ShardingKey: { a: 1 }, ShardingType: 'hash', ReplSize: 0 };
   var hashCL = commCreateCL( db, csName, clName, options, true, false, "create hash cl." );
   var sn1 = db.snapshot( 8, { Name: csName + "." + clName } );
   var sourceGroup = sn1.current().toObj()['CataInfo'][0]['GroupName'];
   println( "createCL " + clName + " at ReplicaGroup:" + sourceGroup + " finished" );

   //split ({Partition:1024} {Partition:2048}) {Partition:3072})
   var tarGroupIndex = -1;
   var stepPar = 1024;
   var partId = 3;
   var lowPar = 0;
   var highPar = 0;
   for( var i = 0; i < partId; i++ )
   {
      tarGroupIndex++;
      if( tarGroupIndex == group_num )
         tarGroupIndex = 0;
      if( grouplist[tarGroupIndex] == sourceGroup )
      {
         i--;
         continue;
      }
      lowPar = i * stepPar;
      highPar = ( i + 1 ) * stepPar;
      hashCL.split( sourceGroup, grouplist[tarGroupIndex], { Partition: lowPar }, { Partition: highPar } );
      println( clName + " split from " + sourceGroup + " to " + grouplist[tarGroupIndex] + " {Partition:" + lowPar + "} {Partition:" + highPar + "}" );
   }

   //insert data
   var inserData = [];
   for( var i = 0; i < rownums; i++ )
   {
      inserData.push( { a: rownums - i, b: i, c: "abcdefghijkl" + i } );
   }
   hashCL.insert( inserData );

   //query1
   //select a,b,c from foo.bar order by a desc
   var sel = hashCL.find( null, { a: 0, b: 'b', c: 'c' } ).sort( { a: -1 } );
   //expected result {a:rownums,...} {a:rownums-1,...} ... {a:1,...}
   checkRec( sel, inserData );
   println( "'select a,b,c from foo.bar order by a desc' finished!" );

   //create index
   hashCL.createIndex( indexName, { b: 1 } );
   println( "create indexes finished!" );

   //query2
   //select b from foo.bar order by b
   var sel = hashCL.find( null, { b: 'b' } ).sort( { b: 1 } ).hint( { "": indexName } );
   //expected result {b:0} {b:1} ... {b:rownums-1}
   var i = 0;
   while( sel.next() )
   {
      var ret = sel.current();
      if( ret.toObj()['b'] != i )
      {
         throw buildException( "main()", null, "failed to run index query, check rc : b=" + ret.toObj()['b'], i, ret.toObj()['b'] );
      }
      i++;
   }
   if( i !== rownums )
   {
      throw "returned record number is : " + i;
   }
   println( "'select b from foo.bar order by b' finished!" );

   commDropCL( db, csName, clName, false, false, "drop cl in the end" );
}
