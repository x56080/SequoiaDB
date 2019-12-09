/*******************************************************************************
*@Description : hint测试：强制走表扫描、hint走不存在的索引
                          强制走原本要走的索引、强制走另外一个索引
*@Modify List : 2014-06-12   xiaojun Hu   Init
                2016-03-17   Ting YU      Modify
*******************************************************************************/
main();

function main ()
{
   try
   {
      var csName = COMMCSNAME;
      var clName = COMMCLNAME;
      var idxName1 = COMMCLNAME + "_a_idx";
      var idxName2 = COMMCLNAME + "_b_idx";

      var clObj = new Collection( csName, clName, { ReplSize: 0 } );
      var cl = clObj.create();
      clObj.createIndex( idxName1, { a: -1 } );
      clObj.createIndex( idxName2, { b: 1 } );

      var recs = [];
      for( var i = 0; i < 100; i++ ) recs.push( { a: i, b: i + 0.95 } );
      clObj.insertRecs( recs );

      println( "---begin to query by hint({'':null}}" );
      var rc = cl.find( { a: { $gte: 0 } } ).sort( { a: 1 } ).hint( { "": null } );
      checkExplain( rc, "" );
      checkRec( rc, recs );

      println( "---begin to query, hinted by non-existed index" );
      var rc = cl.find().sort( { b: 1 } ).hint( { "": "non_existed_index" } );
      checkExplain( rc, idxName2 );
      checkRec( rc, recs );

      println( "---begin to query, hinted by index" );
      var rc = cl.find().sort( { a: 1 } ).hint( { "": idxName1 } );
      checkExplain( rc, idxName1 );
      checkRec( rc, recs );

      println( "---begin to query, hinted by another index" );
      var rc = cl.find().sort( { b: 1 } ).hint( { "": idxName1 } );
      checkExplain( rc, idxName1 );
      checkRec( rc, recs );
   }
   catch( e )
   {
      throw e;
   }
}