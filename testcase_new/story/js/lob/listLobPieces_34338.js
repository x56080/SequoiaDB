/******************************************************************************
@Description : seqDB-34338:listLobPieces() 接口测试
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

   var csName = "cs_34338";
   var pubLobFile = "lob_34338";
   var normalCLName = "normal_34338";
   var hashCLName = "hash_34338";
   var hashCLName2 = "hash_34338_2";
   var mainCLName = "main_34338";

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

      var cursor = cl.listLobPieces();
      checkListLobsResult( cursor, [ groupID1, groupID2 ], [
           '{"Oid":{"$oid":"' + id1 + '"},"Sequence":0,"Length":262144}',
           '{"Oid":{"$oid":"' + id1 + '"},"Sequence":1,"Length":201660}',
           '{"Oid":{"$oid":"' + id2 + '"},"Sequence":0,"Length":262144}',
           '{"Oid":{"$oid":"' + id2 + '"},"Sequence":1,"Length":201660}'
      ]);
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
