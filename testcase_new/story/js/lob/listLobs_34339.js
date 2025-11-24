/******************************************************************************
@Description : seqDB-34339:listLob 优化测试
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

   var csName = "cs_34339";
   var pubLobFile = "lob_34339";
   var normalCLName = "normal_34339";
   var hashCLName = "hash_34339";
   var hashCLName2 = "hash_34339_2";
   var mainCLName = "main_34339";

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

      var id1 = cl.createLobID( "2015-06-05-16.10.33" );
      cl.putLob( pubLobFile, id1 );
      var id2 = cl.createLobID( "2015-06-06-16.10.33" );
      cl.putLob( pubLobFile, id2 );
      var id3 = cl.createLobID( "2015-06-06-17.10.33" );

      var cursor = cl.listLobs(SdbQueryOption().hint({ListPieces:true}));
      checkListLobsResult(cursor, [ groupID1, groupID2 ], [
            '{"Oid":{"$oid":"' + id1 + '"},"Sequence":0,"Length":262144}',
            '{"Oid":{"$oid":"' + id1 + '"},"Sequence":1,"Length":201660}',
            '{"Oid":{"$oid":"' + id2 + '"},"Sequence":0,"Length":262144}',
            '{"Oid":{"$oid":"' + id2 + '"},"Sequence":1,"Length":201660}'
      ]);

      // a 1
      var cursor = cl.listLobs(SdbQueryOption().hint({Oid:id1}));
      checkListLobsResult(cursor, [ groupID1, groupID2 ], [ "{\"Size\":462780,\"Oid\":{\"$oid\":\"" + id1 + "\"},\"Available\":true,\"HasPiecesInfo\":false}" ]);

      // a 2
      var cursor = cl.listLobs(SdbQueryOption().hint({Oid:[id1, id2]}));
      checkListLobsResult(cursor, [ groupID1, groupID2 ], [
            "{\"Size\":462780,\"Oid\":{\"$oid\":\"" + id1 + "\"},\"Available\":true,\"HasPiecesInfo\":false}",
            "{\"Size\":462780,\"Oid\":{\"$oid\":\"" + id2 + "\"},\"Available\":true,\"HasPiecesInfo\":false}"
      ]);

      // b 1
      var cursor = cl.listLobs(SdbQueryOption().cond({Oid:{$oid:id2}}));
      checkListLobsResult(cursor, [ groupID1, groupID2 ], [ "{\"Size\":462780,\"Oid\":{\"$oid\":\"" + id2 + "\"},\"Available\":true,\"HasPiecesInfo\":false}" ]);

      // b 2
      var cursor = cl.listLobs(SdbQueryOption().cond({$or:[ {Oid:{$oid:id1}}, {Oid:{$oid:id2}} ] } ));
      checkListLobsResult(cursor, [ groupID1, groupID2 ], [
            "{\"Size\":462780,\"Oid\":{\"$oid\":\"" + id1 + "\"},\"Available\":true,\"HasPiecesInfo\":false}",
            "{\"Size\":462780,\"Oid\":{\"$oid\":\"" + id2 + "\"},\"Available\":true,\"HasPiecesInfo\":false}"
      ]);

      // b 3
      var cursor = cl.listLobs(SdbQueryOption().cond({$and:[ {Oid:{$oid:id1}}, {Size:{$gt:500000}} ] } ));
      checkListLobsResult(cursor, [], []);

      // b 4
      var cursor = cl.listLobs(SdbQueryOption().cond({$and:[ {Oid:{$oid:id1}}, {Size:{$lt:500000}} ] } ));
      checkListLobsResult(cursor, [ groupID1, groupID2 ], [ "{\"Size\":462780,\"Oid\":{\"$oid\":\"" + id1 + "\"},\"Available\":true,\"HasPiecesInfo\":false}" ]);

      // c
      var cursor = cl.listLobs(SdbQueryOption().hint({ GroupID:[groupID1,groupID2]})) 
      checkListLobsResult(cursor, [ groupID1, groupID2 ], [
            "{\"Size\":462780,\"Oid\":{\"$oid\":\"" + id1 + "\"},\"Available\":true,\"HasPiecesInfo\":false}",
            "{\"Size\":462780,\"Oid\":{\"$oid\":\"" + id2 + "\"},\"Available\":true,\"HasPiecesInfo\":false}"
      ]);

      // d
      var cursor = cl.listLobs(SdbQueryOption().cond({$and:[{GroupID:{$in:[groupID1,groupID2]}}]}));
      checkListLobsResult(cursor, [ groupID1, groupID2 ], [
            "{\"Size\":462780,\"Oid\":{\"$oid\":\"" + id1 + "\"},\"Available\":true,\"HasPiecesInfo\":false}",
            "{\"Size\":462780,\"Oid\":{\"$oid\":\"" + id2 + "\"},\"Available\":true,\"HasPiecesInfo\":false}"
      ]);

      // f 1
      var cursor = cl.listLobs(SdbQueryOption().hint({Oid:[id1, id3],ListPieces:true,GroupID:groupID1}));
      checkListLobsResult2(cursor, [ groupID1 ], [
            '{"Oid":{"$oid":"' + id1 + '"},"Sequence":0,"Length":262144}',
            '{"Oid":{"$oid":"' + id1 + '"},"Sequence":1,"Length":201660}'
      ]);

      // f 2
      var cursor = cl.listLobs(SdbQueryOption().cond({$and:[ {Oid:{$oid:id1}}, {GroupID:groupID2} ] } ));
      checkListLobsResult2(cursor, [ groupID2 ], [ "{\"Size\":462780,\"Oid\":{\"$oid\":\"" + id1 + "\"},\"Available\":true,\"HasPiecesInfo\":false}" ]);

      // f 3
      var cursor = cl.listLobs(SdbQueryOption().cond({Oid:{$oid:id1}}).hint({GroupID:groupID2}));
      checkListLobsResult2(cursor, [ groupID2 ], [ "{\"Size\":462780,\"Oid\":{\"$oid\":\"" + id1 + "\"},\"Available\":true,\"HasPiecesInfo\":false}" ]);

      // f 4
      var cursor = cl.listLobs(SdbQueryOption().cond({Oid:{$oid:id1}}).hint({Oid:{$oid:id1}}));
      checkListLobsResult(cursor, [ groupID1, groupID2 ], [ "{\"Size\":462780,\"Oid\":{\"$oid\":\"" + id1 + "\"},\"Available\":true,\"HasPiecesInfo\":false}" ]);

      // f 5
      var cursor = cl.listLobs(SdbQueryOption().cond({Oid:{$oid:id1}}).hint({Oid:{$oid:id2}}));
      checkListLobsResult(cursor, [], []);

      // f 6
      var cursor = cl.listLobs(SdbQueryOption().cond({GroupID: groupID2}).hint({GroupID: groupID2}));
      checkListLobsResult2(cursor, [ groupID2 ], [
            "{\"Size\":462780,\"Oid\":{\"$oid\":\"" + id1 + "\"},\"Available\":true,\"HasPiecesInfo\":false}",
            "{\"Size\":462780,\"Oid\":{\"$oid\":\"" + id2 + "\"},\"Available\":true,\"HasPiecesInfo\":false}"
      ]);

      // f 7
      var cursor = cl.listLobs(SdbQueryOption().cond({GroupID: groupID1}).hint({GroupID: groupID2}));
      checkListLobsResult(cursor, [], []);

      cl.truncate();
   }

   commDropCS( db, csName, false, "Failed to drop cs in the end-condition." );
   cmd.run( "rm -rf " + pubLobFile + "*" );
}

// lobs from cursor equal espectLobs
function checkListLobsResult( cursor, espectGroups, espectLobs )
{
   var actualLobs = [];
   while ( result = cursor.next() )
   {
      var obj = result.toObj();
      var isInEspect = false;
      for (var i = 0; i < espectGroups.length; i++)
      {
         if ( obj.GroupID == espectGroups[i] )
         {
            isInEspect = true;
            break;
         }
      }
      if ( !isInEspect )
      {
         throw new Error( "Wrong group [" + obj.GroupID + "], " +
                          "which is not in " + JSON.stringify( espectGroups ) );
      }
      delete obj.GroupID;
      delete obj.CreateTime;
      delete obj.ModificationTime;
      actualLobs.push( JSON.stringify( obj ) );
   }
   actualLobs.sort();
   espectLobs.sort();

   var actualString = JSON.stringify( actualLobs );
   var espectString = JSON.stringify( espectLobs );

   if ( actualString != espectString )
   {
      throw new Error( "Wrong result. espect: " + espectString + "; actual: " + actualString );
   }
}

// lobs from cursor are sub set of espectLobs
function checkListLobsResult2( cursor, espectGroups, espectLobs )
{
   var actualLobs = [];
   while ( result = cursor.next() )
   {
      var obj = result.toObj();
      var isInEspect = false;
      for (var i = 0; i < espectGroups.length; i++)
      {
         if ( obj.GroupID == espectGroups[i] )
         {
            isInEspect = true;
            break;
         }
      }
      if ( !isInEspect )
      {
         throw new Error( "Wrong group [" + obj.GroupID + "], " +
                          "which is not in " + JSON.stringify( espectGroups ) );
      }
      delete obj.GroupID;
      delete obj.CreateTime;
      delete obj.ModificationTime;
      actualLobs.push( JSON.stringify( obj ) );
   }
   actualLobs.sort();
   espectLobs.sort();

   for (var i = 0; i < actualLobs; i++)
   {
      var isInEspect = false;
      for (var j = 0; j < espectLobs; j++)
      {
         var actualString = JSON.stringify( actualLobs[i] );
         var espectString = JSON.stringify( espectLobs[j] );
         if ( actualString == espectString )
         {
            isInEspect = true;
            break;
         }
      }
      if ( !isInEspect )
      {
         throw new Error( "Wrong result. espect: " + JSON.stringify( espectLobs ) + "; actual: " + JSON.stringify( actualLobs[i] ) );
      }
   }
}
