/******************************************************************************
@Description : seqDB-34340:lobCount() 接口测试
@Modify list :
2025-11-11 Suqiang Lin Create
****************************************************************************/

main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   if( commGetGroupsNum( db ) < 2 )
   {
      return;
   }

   var csName = "cs_34340";
   var pubLobFile = "lob_34340";
   var normalCLName = "normal_34340";
   var hashCLName = "hash_34340";
   var hashCLName2 = "hash_34340_2";
   var mainCLName = "main_34340";

   var cmd = new Cmd();
   var pwd = cmd.run( "pwd" ).split( "\n" )[0];
   cmd.run( "rm -rf " + pubLobFile + "*" );
   lobGenerateFile( pubLobFile, 3000 );

   var groups = commGetGroups( db );
   var groupName1 = groups[0][0]["GroupName"];
   var groupName2 = groups[1][0]["GroupName"];
   var groupID1 = groups[0][0]["GroupID"];
   var groupID2 = groups[1][0]["GroupID"];

   commDropCS( db, csName, true, "Failed to drop cs in the pre-condition." );
   var cs = db.createCS( csName );
   var normalCL = cs.createCL( normalCLName, { Group: groupName1 } );
   var hashCL = cs.createCL( hashCLName, { ShardingType: "hash", ShardingKey: { a: 1 }, Group: [ groupName1, groupName2 ] } );
   var hashCL2 = cs.createCL( hashCLName2, { ShardingType: "hash", ShardingKey: { a: 1 }, Group: [ groupName1, groupName2 ] } );
   var mainCL = cs.createCL( mainCLName, { IsMainCL: true, ShardingKey: { date: 1 }, LobShardingKeyFormat: "YYYYMMDD" } );

   mainCL.attachCL(csName + "." + hashCLName, { LowBound: { date: "20150605" }, UpBound: { date: "20150606" } });
   mainCL.attachCL(csName + "." + hashCLName2, { LowBound: { date: "20150606" }, UpBound: { date: "20150607" } });

   // normal cl
   var clList = [ normalCL, hashCL, mainCL ];
   for (var clItr = 0; clItr < clList.length; clItr++)
   {
      println( clItr );
      cl = clList[ clItr ];

      var id1 = cl.createLobID( "2015-06-05-16.10.33" );
      cl.putLob( pubLobFile, id1 );
      var id2 = cl.createLobID( "2015-06-06-16.10.33" );
      cl.putLob( pubLobFile, id2 );
      var id3 = cl.createLobID( "2015-06-06-17.10.33" );

      // a
      var count = cl.lobCount();
      assert.equal( count, 2 );

      // b
      var count = cl.lobCount({ Oid: { $oid: id1 } });
      assert.equal( count, 1 );

      var count = cl.lobCount({ Oid: { $oid: id3 } });
      assert.equal( count, 0 );

      // c
      var count = cl.lobCount({ Size: { $lt: 500000 } });
      assert.equal( count, 2 );

      var count = cl.lobCount({ Size: { $gt: 500000 } });
      assert.equal( count, 0 );

      // d
      var count = cl.lobCount().hint({ListPieces:true});
      assert.equal( count, 4 );

      // d
      var count1 = cl.lobCount().hint({GroupID:groupID1});
      var count2 = cl.lobCount().hint({GroupID:groupID2});
      assert.equal( count1 + count2, 2 );

      // f
      var count1 = cl.lobCount({ Oid: { $oid: id1 } }).hint({GroupID:groupID1});
      var count2 = cl.lobCount({ Oid: { $oid: id1 } }).hint({GroupID:groupID2});
      assert.equal( count1 + count2, 1 );

      var count = cl.lobCount({ Oid: { $oid: id1 } }).hint({ListPieces:true});
      assert.equal( count, 2 );

      cl.truncate();
   }

   commDropCS( db, csName, false, "Failed to drop cs in the end-condition." );
   cmd.run( "rm -rf " + pubLobFile + "*" );
}
