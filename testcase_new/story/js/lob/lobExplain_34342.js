/******************************************************************************
@Description : seqDB-34342:lobExplain() 接口测试
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

   var csName = "cs_34342";
   var pubLobFile = "lob_34342";
   var normalCLName = "normal_34342";
   var hashCLName = "hash_34342";
   var hashCLName2 = "hash_34342_2";
   var mainCLName = "main_34342";

   var cmd = new Cmd();
   var pwd = cmd.run( "pwd" ).split( "\n" )[0];
   cmd.run( "rm -rf " + pubLobFile + "*" );
   lobGenerateFile( pubLobFile, 3000 );

   var groups = commGetGroups( db );
   var groupName1 = groups[0][0]["GroupName"];
   var groupName2 = groups[1][0]["GroupName"];
   var groupID1 = groups[0][0]["GroupID"];
   var groupID2 = groups[1][0]["GroupID"];
   var subCLName1 = null;
   var subCLName2 = null;

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

      var id1 = cl.createLobID( "2015-06-05-16.10.33" );
      cl.putLob( pubLobFile, id1 );
      var id2 = cl.createLobID( "2015-06-06-16.10.33" );
      cl.putLob( pubLobFile, id2 );
      var id3 = cl.createLobID( "2015-06-06-17.10.33" );

      // a
      var result = cl.lobExplain( id1 ).toObj();
      if ( clItr == 2 )
      {
         subCLName1 = csName + "." + hashCLName;
         subCLName2 = csName + "." + hashCLName2;
      }
      else
      {
         subCLName1 = null;
         subCLName2 = null;
      }
      checkResult(result, id1, [ groupID1, groupID2 ], '{"LobPageSize":262144,"Exist":true,"Offset":0,"Length":462780,"PiecesNum":2}', subCLName1);

      // b
      var result = cl.lobExplain( id3 ).toObj();
      checkResult(result, id3, [ groupID1, groupID2 ], '{"LobPageSize":262144,"Exist":false,"Offset":0,"Length":524288,"PiecesNum":3}', subCLName2);
      // c
      var result = cl.lobExplain( id1, true ).toObj();
      checkResult(result, id1, [ groupID1, groupID2 ], '{"LobPageSize":262144,"Exist":true,"Offset":0,"Length":462780,"PiecesNum":2}', subCLName1);
      // d
      var result = cl.lobExplain( id3, true, { Offset: 300, Length: 100 } ).toObj();
      checkResult(result, id3, [ groupID1, groupID2 ], '{"LobPageSize":262144,"Exist":false,"Offset":300,"Length":100,"PiecesNum":1}', subCLName2);
      var result = cl.lobExplain( id3, true, { Offset: 300, Length: 1000000 } ).toObj();
      checkResult(result, id3, [ groupID1, groupID2 ], '{"LobPageSize":262144,"Exist":false,"Offset":300,"Length":1000000,"PiecesNum":4}', subCLName2);
      var result = cl.lobExplain( id3, true, { Offset: 300, Length: 10000000000 } ).toObj();
      checkResult(result, id3, [ groupID1, groupID2 ], '{"LobPageSize":262144,"Exist":false,"Offset":300,"Length":2147483648,"PiecesNum":8193}', subCLName2);
      var result = cl.lobExplain( id3, true, { Offset: 300, Length: 0 } ).toObj();
      checkResult(result, id3, [ groupID1, groupID2 ], '{"LobPageSize":262144,"Exist":false,"Offset":300,"Length":524288,"PiecesNum":3}', subCLName2);
      var result = cl.lobExplain( id3, true, { Offset: 0, Length: 100 } ).toObj();
      checkResult(result, id3, [ groupID1, groupID2 ], '{"LobPageSize":262144,"Exist":false,"Offset":0,"Length":100,"PiecesNum":1}', subCLName2);
      var result = cl.lobExplain( id3, true, { Offset: 0, Length: -1 } ).toObj();
      checkResult(result, id3, [ groupID1, groupID2 ], '{"LobPageSize":262144,"Exist":false,"Offset":0,"Length":524288,"PiecesNum":3}', subCLName2);
      var result = cl.lobExplain( id3, true, { Offset: -1, Length: 100 } ).toObj();
      checkResult(result, id3, [ groupID1, groupID2 ], '{"LobPageSize":262144,"Exist":false,"Offset":0,"Length":100,"PiecesNum":1}', subCLName2);
      var result = cl.lobExplain( id3, true, { Offset: 100000, Length: 100 } ).toObj();
      checkResult(result, id3, [ groupID1, groupID2 ], '{"LobPageSize":262144,"Exist":false,"Offset":100000,"Length":100,"PiecesNum":1}', subCLName2);

      cl.truncate();
   }

   commDropCS( db, csName, false, "Failed to drop cs in the end-condition." );
   cmd.run( "rm -rf " + pubLobFile + "*" );
}

function checkResult( actual, oid, groups, espectString, subCLName )
{
   assert.equal( actual.Oid, oid );
   delete actual.Oid;
   checkInGroups( actual.GroupID, groups );
   delete actual.GroupID;
   if ( subCLName )
   {
      assert.equal( actual.SubCLName, subCLName );
      delete actual.SubCLName;
   }

   var total = 0;
   for (var i = 0; i < actual.Location.length; i++)
   {
      checkInGroups( actual.Location[i].GroupID, groups );
      delete actual.Location[i].GroupID;
      total += actual.Location[i].PiecesNum;
      delete actual.Location[i].PiecesNum;
   }
   delete actual.Location;
   assert.equal( actual.PiecesNum, total );

   actualString = JSON.stringify( actual );
   assert.equal( actualString, espectString );
}

function checkInGroups( groupID, groups )
{
   var inGroups = false;
   for (var i = 0; i < groups.length; i++)
   {
      if ( groupID == groups[i] )
      {
         inGroups = true;
         break;
      }
   }
   if ( !inGroups )
   {
      throw new Error( "group[" + groupID + "] is not in " +
                       JSON.stringify( groups ) );
   }
}
