/******************************************************************************
*@Description : test SdbQueryOption
*               TestLink :   seqDB-15745:���ӱ���ִ��sort/limit/skip������ϲ�ѯ
                             seqDB-15746:���ӱ���ִ��cond/update������ϲ�ѯ
                             seqDB-15747:���ӱ���ִ��cond/sel/sort/hint������ϲ�ѯ
                             seqDB-15748:���ӱ���ִ��cond/remove������ϲ�ѯ
*@auhor       : CSQ 
******************************************************************************/

function main ()
{
   if( commGetGroupsNum( db ) < 2 )
   {
      return;
   }
   try
   {
      commDropCS( db, COMMCSNAME + "main15745", true, "drop CS " + COMMCSNAME + "main15745" );
      commDropCS( db, COMMCSNAME + "sub15745", true, "drop CS " + COMMCSNAME + "sub15745" );
   } catch( e ) { }
   var groups = commGetGroups( db );
   var srcGroupName = groups[0][0].GroupName;
   var destGroupName = groups[1][0].GroupName;
   var varCS = commCreateCS( db, COMMCSNAME + "main15745", true, "create CS" );
   var varCL = varCS.createCL( COMMCLNAME + "main15745", { IsMainCL: true, ShardingKey: { a: 1 }, ShardingType: "range" } );
   //�ӱ�1
   var subCS = commCreateCS( db, COMMCSNAME + "sub15745", true, "create CS1" );
   var subcl1 = subCS.createCL( COMMCLNAME + "subcl1_15745", { ShardingKey: { a: 1 }, ShardingType: "hash", Partition: 1024 } );
   //�ӱ�2
   var subcl2 = subCS.createCL( COMMCLNAME + "subcl2_15745", { ShardingKey: { a: 1 }, ShardingType: "hash", Partition: 1024 } );
   //����
   attachCL( varCL, COMMCSNAME + "sub15745." + COMMCLNAME + "subcl1_15745", { LowBound: { a: 0 }, UpBound: { a: 50 } } );
   attachCL( varCL, COMMCSNAME + "sub15745." + COMMCLNAME + "subcl2_15745", { LowBound: { a: 50 }, UpBound: { a: 100 } } );

   varCL.createIndex( "bindex", { b: 1 }, false );
   insertRecord( varCL );
   testcombination15745( varCL );
   testcombination15746( varCL );
   testcombination15747( varCL );
   testcombination15748( varCL );

   try
   {
      commDropCS( db, COMMCSNAME + "main15745", true, "drop CS " + COMMCSNAME + "main15745" );
      commDropCS( db, COMMCSNAME + "sub15745", true, "drop CS " + COMMCSNAME + "sub15745" );
   } catch( e ) { }
}

//ָ��sort/limit/skip��ѯ��������ѯ���ݸ��Ƕ���ӱ���
function testcombination15745 ( varCL )
{
   var cur = varCL.find( new SdbQueryOption().sort( { a: 1 } ).skip( 45 ).limit( 10 ) );
   var expFindResult = [{ "_id": 45, "a": 45, "b": 45, "c": -45 },
   { "_id": 46, "a": 46, "b": 46, "c": -46 },
   { "_id": 47, "a": 47, "b": 47, "c": -47 },
   { "_id": 48, "a": 48, "b": 48, "c": -48 },
   { "_id": 49, "a": 49, "b": 49, "c": -49 },
   { "_id": 50, "a": 50, "b": 50, "c": -50 },
   { "_id": 51, "a": 51, "b": 51, "c": -51 },
   { "_id": 52, "a": 52, "b": 52, "c": -52 },
   { "_id": 53, "a": 53, "b": 53, "c": -53 },
   { "_id": 54, "a": 54, "b": 54, "c": -54 }
   ];
   checkRec( cur, expFindResult );
}

//ָ��cond/update��ѯ��������ѯ���ݸ��Ƕ���ӱ���
//��db.foo.bar.find(new SdbQueryOption().
function testcombination15746 ( varCL )
{
   var cur = varCL.find( new SdbQueryOption().sort( { a: 1 } ).cond( { $and: [{ a: { $gte: 45 } }, { b: { $lte: 54 } }] } ).update( { $set: { c: 1 } } ) );
   while( cur.next() )
   {
      cur.current();
   }
   var expFindResult = [{ "_id": 45, "a": 45, "b": 45, "c": 1 },
   { "_id": 46, "a": 46, "b": 46, "c": 1 },
   { "_id": 47, "a": 47, "b": 47, "c": 1 },
   { "_id": 48, "a": 48, "b": 48, "c": 1 },
   { "_id": 49, "a": 49, "b": 49, "c": 1 },
   { "_id": 50, "a": 50, "b": 50, "c": 1 },
   { "_id": 51, "a": 51, "b": 51, "c": 1 },
   { "_id": 52, "a": 52, "b": 52, "c": 1 },
   { "_id": 53, "a": 53, "b": 53, "c": 1 },
   { "_id": 54, "a": 54, "b": 54, "c": 1 }
   ];
   var cur = varCL.find( new SdbQueryOption().sort( { a: 1 } ).cond( { $and: [{ a: { $gte: 45 } }, { b: { $lte: 54 } }] } ) );
   checkRec( cur, expFindResult );
}

//ָ��cond/sel/sort/hint��ѯ��������ѯ���ݸ��Ƕ���ӱ���
//��db.foo.bar.find(new SdbQueryOption().cond().sel().sort().hint()������ƥ���ֶΰ����������ͷǷ������ֶ� 
function testcombination15747 ( varCL )
{
   var cur = varCL.find( new SdbQueryOption().cond( { $and: [{ a: { $gte: 45 } }, { b: { $lt: 55 } }] } ).sel( { _id: { $include: 1 }, a: { $include: 1 }, b: { $include: 1 } } ).sort( { b: 1 } ).hint( { "": "bindex" } ) );
   var expFindResult = [{ "_id": 45, "a": 45, "b": 45 },
   { "_id": 46, "a": 46, "b": 46 },
   { "_id": 47, "a": 47, "b": 47 },
   { "_id": 48, "a": 48, "b": 48 },
   { "_id": 49, "a": 49, "b": 49 },
   { "_id": 50, "a": 50, "b": 50 },
   { "_id": 51, "a": 51, "b": 51 },
   { "_id": 52, "a": 52, "b": 52 },
   { "_id": 53, "a": 53, "b": 53 },
   { "_id": 54, "a": 54, "b": 54 }
   ];
   checkRec( cur, expFindResult );
}
//ָ��cond/remove��ѯ��������ѯ���ݸ��Ƕ���ӱ���
//��db.foo.bar.find(new SdbQueryOption().cond().remove()
function testcombination15748 ( varCL )
{
   var cur = varCL.find( new SdbQueryOption().cond( { $or: [{ a: { $lt: 45 } }, { b: { $gte: 55 } }] } ).remove() );
   while( cur.next() )
   {
      var ret = cur.current();
   }
   var cur = varCL.find( new SdbQueryOption().sort( { _id: 1 } ) );
   var expFindResult = [{ "_id": 45, "a": 45, "b": 45, "c": 1 },
   { "_id": 46, "a": 46, "b": 46, "c": 1 },
   { "_id": 47, "a": 47, "b": 47, "c": 1 },
   { "_id": 48, "a": 48, "b": 48, "c": 1 },
   { "_id": 49, "a": 49, "b": 49, "c": 1 },
   { "_id": 50, "a": 50, "b": 50, "c": 1 },
   { "_id": 51, "a": 51, "b": 51, "c": 1 },
   { "_id": 52, "a": 52, "b": 52, "c": 1 },
   { "_id": 53, "a": 53, "b": 53, "c": 1 },
   { "_id": 54, "a": 54, "b": 54, "c": 1 }
   ];
   checkRec( cur, expFindResult );
}

function insertRecord ( varCL )
{
   for( var i = 0; i < 100; i++ )
   {
      varCL.insert( { _id: i, a: i, b: i, c: -i } );
   }
}

main();