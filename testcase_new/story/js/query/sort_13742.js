/******************************************************************************
@Description : 1. range-cl sort
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

   var indexName = "index13742";
   var rownums = 10000;
   var csName = COMMCSNAME;
   var clName = "cl13742";

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

   commDropCL( db, csName, clName, true, true, "drop cl in the beginning" );
   var options = { ShardingKey: { a: 1 }, ShardingType: 'range', ReplSize: 0 };
   var rangeCL = commCreateCLByOption( db, csName, clName, options, true, false, "create range cl." );
   var sn1 = db.snapshot( 8, { Name: csName + "." + clName } );
   var sourceGroup = sn1.current().toObj()['CataInfo'][0]['GroupName'];
   println( "createCL " + clName + " at ReplicaGroup:" + sourceGroup + " finished" );

   //split ({a:0} {a:5000})
   var tarGroupIndex = -1;
   var stepId = 5000;
   var partId = rownums / stepId;
   var lowId = 0;
   var highId = 0;
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
      lowId = ( i - 1 ) * stepId;
      highId = i * stepId;
      rangeCL.split( sourceGroup, grouplist[tarGroupIndex], { a: lowId }, { a: highId } );
      println( COMMCLNAME + " split from " + sourceGroup + " to " + grouplist[tarGroupIndex] + " {a:" + lowId + "} {a:" + highId + "}" );
   }
   println( "split rangeCL success!" );

   //insert data
   var records = [];
   for( var i = 0; i < rownums; i++ )
   {
      records.push( { a: rownums - i, b: i, c: "abcdefghijkl" + i } );
   }
   rangeCL.insert( records );
   println( "insert-data into rangeCL succ!" );

   //query1
   //select a,b,c from foo.bar order by a desc
   var sel = rangeCL.find( null, { a: 0, b: 'b', c: 'c' } ).sort( { a: -1 } );
   checkRec( sel, records );
   println( "'select a,b,c from foo.bar order by a desc' finished!" );

   //create index
   rangeCL.createIndex( indexName, { b: 1 } );
   println( "create indexes finished!" );

   //query2
   //select b from foo.bar order by b
   var sel = rangeCL.find( null, { b: 'b' } ).sort( { b: 1 } ).hint( { "": indexName } );
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
