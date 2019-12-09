/*******************************************************************************
*@Description : 普通表的访问计划
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

      var collection = new Collection( csName, clName, { ReplSize: 0 } );
      var cl = collection.create();
      var groupList = collection.getGroups();

      println( "---begin to: cl.find().explain()" );
      var rc = cl.find().explain();
      var expOpt = { ScanType: "tbscan", IndexName: "", Query: { "$and": [] }, groupList: groupList };
      checkExplain( rc, expOpt );

      println( "---begin to: cl.find({a:{$et:1}}).explain()" );
      var rc = cl.find( { a: { $et: 1 } } ).explain();
      var expOpt = { ScanType: "tbscan", IndexName: "", Query: { "$and": [{ "a": { "$et": 1 } }] }, groupList: groupList };
      checkExplain( rc, expOpt );

      cl.createIndex( indexName, { a: 1 } );

      println( "---begin to: cl.find({a:{$et:1}}).explain()" );
      var rc = cl.find( { a: { $et: 1 } } ).explain();
      var expOpt = { ScanType: "ixscan", IndexName: indexName, Query: { "$and": [{ "a": { "$et": 1 } }] }, groupList: groupList };
      checkExplain( rc, expOpt );

      println( "---begin to: cl.find().sort({a:1}).explain()" );
      var rc = cl.find().sort( { a: 1 } ).explain();
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
