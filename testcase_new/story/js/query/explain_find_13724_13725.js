/*******************************************************************************
*@Description : 切分表的访问计划
*@Modify List : 2014-07-14  pusheng Ding  Init
                2016-3-17   Ting YU       modify
*******************************************************************************/
main();

function main ()
{
   try
   {
      var csName = COMMCSNAME;
      var clName = COMMCLNAME;
      var indexName = COMMCLNAME + '_idx';
      var sdIdxName = "$shard";

      if( commIsStandalone( db ) )
      {
         println( " Deploy mode is standalone!" );
         return;
      }
      if( commGetGroupsNum( db ) < 2 )
      {
         println( "This testcase needs at least 2 groups to split cl!" );
         return;
      }

      var groups = select2RG();
      var clOpt = { ReplSize: 0, ShardingKey: { a: 1 }, ShardingType: "range", Group: groups.srcRG };

      var collection = new Collection( csName, clName, clOpt );
      var cl = collection.create();

      println( "---begin to split by {a:0} {a:100}" );
      cl.split( groups.srcRG, groups.tgtRG, { a: 0 }, { a: { "$maxKey": 1 } } );
      var groupList = collection.getGroups();

      println( "---begin to: cl.find().explain()" );
      var rc = cl.find().explain();
      var expOpt = { ScanType: "tbscan", IndexName: "", Query: { "$and": [] }, groupList: groupList };
      checkExplain( rc, expOpt );

      println( "---begin to: cl.find({a:{$gt:-1}}).explain()" );
      var rc = cl.find( { a: { $gt: -1 } } ).explain();
      var expOpt = { ScanType: "tbscan", IndexName: "", Query: { "$and": [{ "a": { "$gt": -1 } }] }, groupList: groupList };
      checkExplain( rc, expOpt );

      println( "---begin to: cl.find({a:{$gt:1}}).explain()" );
      var rc = cl.find( { a: { $gt: 1 } } ).explain();
      var expOpt = { ScanType: "tbscan", IndexName: "", Query: { "$and": [{ "a": { "$gt": 1 } }] }, groupList: [groups.tgtRG] };
      checkExplain( rc, expOpt );

      println( "---begin to: cl.find().sort({a:1}).explain()" );
      var rc = cl.find().sort( { a: 1 } ).explain();
      var expOpt = { ScanType: "ixscan", IndexName: sdIdxName, Query: { "$and": [] }, groupList: groupList };
      checkExplain( rc, expOpt );

      cl.createIndex( indexName, { b: 1 } );

      println( "---begin to: cl.find({b:{$et:1}}).explain()" );
      var rc = cl.find( { b: { $et: 1 } } ).explain();
      var expOpt = { ScanType: "ixscan", IndexName: indexName, Query: { "$and": [{ "b": { "$et": 1 } }] }, groupList: groupList };
      checkExplain( rc, expOpt );

      println( "---begin to: cl.find().sort({b:1}).explain()" );
      var rc = cl.find().sort( { b: 1 } ).explain();
      var expOpt = { ScanType: "ixscan", IndexName: indexName, Query: { "$and": [] }, groupList: groupList };
      checkExplain( rc, expOpt );

      println( '---begin to: cl.find().hint({"":' + indexName + '}).explain()' );
      var rc = cl.find().hint( { "": indexName } ).explain();
      var expOpt = { ScanType: "ixscan", IndexName: indexName, Query: { "$and": [] }, groupList: groupList };
      checkExplain( rc, expOpt );
   }
   catch( e )
   {
      throw e;
   }
}

function checkExplain ( actRtn, expOpt )
{
   println( "---begin to check explain" );

   var groupList = [];
   while( actRtn.next() )
   {
      var atcObj = actRtn.current().toObj();

      if( atcObj.ScanType !== expOpt.ScanType )
         throw buildException( "check ScanType", "", "get 'ScanType' value", expOpt.ScanType, atcObj.ScanType );

      if( atcObj.IndexName !== expOpt.IndexName )
         throw buildException( "check ScanType", "", "get 'IndexName' value", expOpt.IndexName, atcObj.IndexName );

      var query = JSON.stringify( atcObj.Query );
      var expQuery = JSON.stringify( expOpt.Query );
      if( query !== expQuery )
         throw buildException( "check ScanType", "", "get 'scanType' value", expQuery, query );

      groupList.push( atcObj.GroupName );
   }

   if( groupList.sort().toString() !== expOpt.groupList.sort().toString() )
      throw buildException( "check group", "", "get 'grouplist'", expOpt.groupList, groupList );

}
