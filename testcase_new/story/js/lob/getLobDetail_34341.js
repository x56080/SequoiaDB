/******************************************************************************
@Description : seqDB-34341:getLobDetail() 优化测试
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

   var csName = "cs_34341";
   var pubLobFile = "lob_34341";
   var normalCLName = "normal_34341";
   var hashCLName = "hash_34341";
   var hashCLName2 = "hash_34341_2";
   var mainCLName = "main_34341";

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

   var clList = [ normalCL, hashCL, mainCL ];
   for (var clItr = 0; clItr < clList.length; clItr++)
   {
      println( clItr );
      cl = clList[ clItr ];
      cl.truncate();

      var id1 = cl.createLobID( "2015-06-05-16.10.33" );
      cl.putLob( pubLobFile, id1 );
      var id2 = cl.createLobID( "2015-06-06-16.10.33" );
      cl.putLob( pubLobFile, id2 );

      var result = cl.getLobDetail( id1, false ).toObj();
      assert.equal( result.GroupID == groupID1 ||
                    result.GroupID == groupID2,
                    true );
      assert.equal( result.PiecesNum > 0,
                    true );
      assert.equal( result.Location[0].GroupID == groupID1 ||
                    result.Location[0].GroupID == groupID2,
                    true );
      assert.equal( result.Location[0].PiecesNum > 0,
                    true );
      if ( clItr == 2 ) // is main cl
      {
          assert.equal( result.SubCLName == (csName + "." + hashCLName),
                        true );
      }
      else
      {
          assert.equal( result.SubCLName == undefined, true );
      }

      var result = cl.getLobDetail( id1, true ).toObj();
      assert.equal( result.GroupID == groupID1 ||
                    result.GroupID == groupID2,
                    true );
      assert.equal( result.PiecesNum > 0,
                    true );
      assert.equal( result.Location[0].GroupID == groupID1 ||
                    result.Location[0].GroupID == groupID2,
                    true );
      assert.equal( result.Location[0].PiecesNum > 0,
                    true );
      assert.equal( result.Location[0].Pieces.length ==
                    result.Location[0].PiecesNum,
                    true );
      if ( clItr == 2 ) // is main cl
      {
          assert.equal( result.SubCLName == (csName + "." + hashCLName),
                        true );
      }
      else
      {
          assert.equal( result.SubCLName == undefined, true );
      }
   }

   commDropCS( db, csName, false, "Failed to drop cs in the end-condition." );
   cmd.run( "rm -rf " + pubLobFile + "*" );
}

