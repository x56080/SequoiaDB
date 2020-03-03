/******************************************************************************
*@Description : seqDB-21860:子表开启压缩类型，分别插入不同的数据量，指定ShowMainCLMode为main，查询集合快照 
                seqDB-21861:子表开启压缩类型，分别插入不同的数据量（都需大于64M），指定ShowMainCLMode为main，查询集合快照
*@author      : Zhao xiaoni
*@Date        : 2020-02-20
******************************************************************************/
testConf.skipStandAlone = true;

//SEQUOIADBMAINSTREAM-5525
//main( test );

function test()
{
   var mainCLName = "mainCL_21860_21861";
   var subCLName1 = "subCL_21860_21861_1";
   var subCLName2 = "subCL_21860_21861_2";
   var groupName = commGetGroups(db)[0][0].GroupName;
   
   commDropCL ( db, COMMCSNAME, mainCLName );
   commDropCL ( db, COMMCSNAME, subCLName1 );
   commDropCL ( db, COMMCSNAME, subCLName2 );
   var mainCL = commCreateCL ( db, COMMCSNAME, mainCLName, { IsMainCL: true, ShardingKey: { a: 1 }, ShardingType: "range" } );
   var subCL1 = commCreateCL ( db, COMMCSNAME, subCLName1, { ShardingKey: { a: 1 }, ShardingType: "range", Compressed: true, Group: groupName } );
   var subCL2 = commCreateCL ( db, COMMCSNAME, subCLName2, { ShardingKey: { a: 1 }, ShardingType: "hash", Compressed: true, Group: groupName } );
   mainCL.attachCL( COMMCSNAME + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 1000 } } );
   mainCL.attachCL( COMMCSNAME + "." + subCLName2, { LowBound: { a: 1000 }, UpBound: { a: 2000 } } );

   var arr = new Array( 1024*1024 );
   arr = arr.join( "a" );
   for( var i = 0; i < 101; i++ )
   {
      subCL1.insert( { a: i, b:  arr } );
   }

   var expResult = { DictionaryCreated: false };
   var snapshotOption = "/*+use_option( ShowMainCLMode, main )*/";
   var cursor = db.exec( 'select * from $SNAPSHOT_CL where Name = "' + COMMCSNAME + '.' + mainCLName + '" ' + snapshotOption );
   checkParameters( cursor, expResult );
   
   for( var i = 0; i < 101; i++ )
   {
      subCL2.insert( { a: i, b: arr } );
   }
   expResult = { DictionaryCreated: true };
   cursor = db.exec( 'select * from $SNAPSHOT_CL where Name = "' + COMMCSNAME + '.' + mainCLName + '" ' + snapshotOption );
   checkParameters( cursor, expResult );

   commDropCL ( db, COMMCSNAME, mainCLName, false, false );
}

