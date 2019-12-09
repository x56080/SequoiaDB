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

      var csName = COMMCSNAME;
      var mainclName = COMMCLNAME + "_maincl";
      var subclName1 = COMMCLNAME + "_subcl1";
      var subclName2 = COMMCLNAME + "_subcl2";
      var indexName = COMMCLNAME + '_idx';
      var sdIdxName = "$shard";

      //create main cl
      var clOpt = { IsMainCL: true, ShardingKey: { mk: 1 }, ShardingType: "range", ReplSize: 0 };
      var collection = new Collection( csName, mainclName, clOpt );
      var cl = collection.create();

      //create sub cl      
      var groups = select2RG();
      var clOpt = { ShardingKey: { sk: 1 }, ShardingType: "hash", ReplSize: 0, Group: groups.srcRG };
      var subcl1 = new Collection( csName, subclName1, clOpt ).create();
      subcl1.split( groups.srcRG, groups.tgtRG, 50 );
      var subcl2 = new Collection( csName, subclName2, clOpt ).create();
      subcl2.split( groups.srcRG, groups.tgtRG, 50 );

      //attach cl
      cl.attachCL( csName + "." + subclName1, { LowBound: { mk: 0 }, UpBound: { mk: 100 } } );
      cl.attachCL( csName + "." + subclName2, { LowBound: { mk: 100 }, UpBound: { mk: 200 } } );

      var subclList = [csName + '.' + subclName1, csName + '.' + subclName2];
      var groupList = [groups.srcRG, groups.tgtRG];

      //explain test
      println( "---begin to: cl.find().explain()" );
      var rc = cl.find().explain();
      var expOpt = { ScanType: "tbscan", IndexName: "", Query: { "$and": [] }, groupList: groupList, subclList: subclList };
      checkExplain( rc, expOpt );

      println( "---begin to: cl.find({sk:{$gt:-1}}).explain()" );
      var rc = cl.find( { sk: { $gt: -1 } } ).explain();
      var expOpt = {
         ScanType: "tbscan", IndexName: "", Query: { "$and": [{ "sk": { "$gt": -1 } }] },
         groupList: groupList, subclList: subclList
      };
      checkExplain( rc, expOpt );

      println( "---begin to: cl.find({mk:{$gt:101}}).explain()" );
      var rc = cl.find( { mk: { $gt: 101 } } ).explain();
      var expOpt = {
         ScanType: "tbscan", IndexName: "", Query: { "$and": [{ "mk": { "$gt": 101 } }] },
         groupList: groupList, subclList: [csName + '.' + subclName2]
      };
      checkExplain( rc, expOpt );

      println( "---begin to: cl.find().sort({sk:1}).explain()" );
      var rc = cl.find().sort( { sk: 1 } ).explain();
      var expOpt = {
         ScanType: "ixscan", IndexName: sdIdxName, Query: { "$and": [] },
         groupList: groupList, subclList: subclList
      };
      checkExplain( rc, expOpt );

      cl.createIndex( indexName, { b: 1 } );

      println( "---begin to: cl.find({b:{$et:1}}).explain()" );
      var rc = cl.find( { b: { $et: 1 } } ).explain();
      var expOpt = {
         ScanType: "ixscan", IndexName: indexName, Query: { "$and": [{ "b": { "$et": 1 } }] },
         groupList: groupList, subclList: subclList
      };
      checkExplain( rc, expOpt );

      println( "---begin to: cl.find().sort({b:1}).explain()" );
      var rc = cl.find().sort( { b: 1 } ).explain();
      var expOpt = {
         ScanType: "ixscan", IndexName: indexName, Query: { "$and": [] },
         groupList: groupList, subclList: subclList
      };
      checkExplain( rc, expOpt );

      println( '---begin to: cl.find().hint({"":' + indexName + '}).explain()' );
      var rc = cl.find().hint( { "": indexName } ).explain();
      var expOpt = {
         ScanType: "ixscan", IndexName: indexName, Query: { "$and": [] },
         groupList: groupList, subclList: subclList
      };
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
      var groupObj = actRtn.current().toObj();
      groupList.push( groupObj.GroupName );

      var subclList = [];
      var subclObj = groupObj.SubCollections;
      for( var i in subclObj )
      {
         var atcObj = subclObj[i];
         subclList.push( atcObj.Name );

         if( atcObj.ScanType !== expOpt.ScanType )
            throw buildException( "check ScanType", "", "get 'ScanType' value", expOpt.ScanType, atcObj.ScanType );

         if( atcObj.IndexName !== expOpt.IndexName )
            throw buildException( "check ScanType", "", "get 'IndexName' value", expOpt.IndexName, atcObj.IndexName );

         var query = JSON.stringify( atcObj.Query );
         var expQuery = JSON.stringify( expOpt.Query );
         if( query !== expQuery )
            throw buildException( "check ScanType", "", "get 'scanType' value", expQuery, query );
      }

      if( subclList.sort().toString() !== expOpt.subclList.sort().toString() )
         throw buildException( "check subcl", "", "get cl 'Name'", expOpt.subclList, subclList );
   }

   if( groupList.sort().toString() !== expOpt.groupList.sort().toString() )
      throw buildException( "check group", "", "get 'grouplist'", expOpt.groupList, groupList );

}
