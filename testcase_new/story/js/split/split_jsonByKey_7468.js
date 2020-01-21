/*******************************************************************************
*@description : seqDB-7468:json格式插入数据后切分到不同分区组中
*@author : 2015-5-28  XiaoJun Hu init; 2020-1-14 XiaoNi Huang modify
********************************************************************************/
try
{
   main();
}
catch( e )
{
   if( e.constructor === Error )
   {
      println( e.stack );
   }
   throw e;
}

function main ()
{
   if( true == commIsStandalone( db ) )
   {
      println( "---Is standalone." );
      return;
   }

   if( commGetGroupsNum( db ) < 2 )
   {
      println( "---Least two groups" );
      return;
   }

   var groupNames = commGetDataGroupNames( db );
   var srcGroupName = groupNames[0];
   var dstGroupName = groupNames[1];
   var clName = CHANGEDPREFIX + "_split_7468";
   var recsNum = 1000;

   commDropCL( db, COMMCSNAME, clName, true, true, "drop cl in the begin" )
   var options = { "ShardingKey": { "obj": 1 }, "ShardingType": "range", "Group": srcGroupName };
   var cl = commCreateCL( db, COMMCSNAME, clName, options );

   // insert
   var docs = [];
   for( var i = 0; i < recsNum; i++ )
   {
      var doc = { "a": i, "obj": { "a": i, "b": "test" + i } }
      docs.push( doc );
   }
   cl.insert( docs );

   // split
   cl.split( srcGroupName, dstGroupName, docs[recsNum / 2], docs[recsNum] );

   // check result
   var sort = { a: 1 };
   var cursor = cl.find( {}, { "_id": { "$include": 0 } } ).sort( sort );
   commCompareResults( cursor, docs );
   checkResultsInGroup( srcGroupName, COMMCSNAME, clName, docs.slice( 0, recsNum / 2 ), sort );
   checkResultsInGroup( dstGroupName, COMMCSNAME, clName, docs.slice( recsNum / 2, recsNum ), sort );

   commDropCL( db, COMMCSNAME, clName, false, false, "drop cl in the end" )
}
