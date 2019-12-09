/******************************************************************************
*@Description : test SdbQueryOption
*               TestLink :   seqDB-15741:��������ִ��sort/limit/skip������ϲ�ѯ
                             seqDB-15742:��������ִ��cond/update������ϲ�ѯ
                             seqDB-15743:��������ִ��cond/sel/sort/hint������ϲ�ѯ
                             seqDB-15744:��������ִ��cond/remove������ϲ�ѯ
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
      commDropCS( db, COMMCSNAME + "15741", true, "drop CS " + COMMCSNAME + "15741" );
   } catch( e ) { }
   var groups = commGetGroups( db );
   var srcGroupName = groups[0][0].GroupName;
   var destGroupName = groups[1][0].GroupName;
   var varCS = commCreateCS( db, COMMCSNAME + "15741", true, "create CS" );
   var varCL = varCS.createCL( COMMCLNAME + "15741", { ShardingKey: { a: 1 }, ShardingType: "hash", Group: srcGroupName } );
   varCL.createIndex( "bindex", { b: 1 }, false );
   insertRecord( varCL );
   varCL.split( srcGroupName, destGroupName, 50 );
   testcombination15741( varCL );
   testcombination15742( varCL );
   testcombination15743( varCL );
   testcombination15744( varCL );

   try
   {
      commDropCS( db, COMMCSNAME + "15741", true, "drop CS " + COMMCSNAME + "15741" );
   } catch( e ) { }
}

//sort/limit/skip��ѯ���Ƕ����
function testcombination15741 ( varCL )
{
   var cur = varCL.find( new SdbQueryOption().sort( { a: 1 } ).skip( 5 ).limit( 20 ) );
   var expFindResult = [{ "_id": 5, "a": 5, "b": 5, "c": -5 },
   { "_id": 6, "a": 6, "b": 6, "c": -6 },
   { "_id": 7, "a": 7, "b": 7, "c": -7 },
   { "_id": 8, "a": 8, "b": 8, "c": -8 },
   { "_id": 9, "a": 9, "b": 9, "c": -9 },
   { "_id": 10, "a": 10, "b": 10, "c": -10 },
   { "_id": 11, "a": 11, "b": 11, "c": -11 },
   { "_id": 12, "a": 12, "b": 12, "c": -12 },
   { "_id": 13, "a": 13, "b": 13, "c": -13 },
   { "_id": 14, "a": 14, "b": 14, "c": -14 },
   { "_id": 15, "a": 15, "b": 15, "c": -15 },
   { "_id": 16, "a": 16, "b": 16, "c": -16 },
   { "_id": 17, "a": 17, "b": 17, "c": -17 },
   { "_id": 18, "a": 18, "b": 18, "c": -18 },
   { "_id": 19, "a": 19, "b": 19, "c": -19 },
   { "_id": 20, "a": 20, "b": 20, "c": -20 },
   { "_id": 21, "a": 21, "b": 21, "c": -21 },
   { "_id": 22, "a": 22, "b": 22, "c": -22 },
   { "_id": 23, "a": 23, "b": 23, "c": -23 },
   { "_id": 24, "a": 24, "b": 24, "c": -24 }
   ];
   checkRec( cur, expFindResult );
}

//find(new SdbQueryOption().cond().update()������ƥ���ֶΰ����������ͷǷ������ֶ� 
function testcombination15742 ( varCL )
{
   var cur = varCL.find( new SdbQueryOption().cond( { $and: [{ a: { $gte: 15 } }, { b: { $lte: 20 } }] } ).update( { $set: { c: 1 } } ).sort( { a: 1 } ) );
   var expFindResult1 = [{ "_id": 15, "a": 15, "b": 15, "c": -15 },
   { "_id": 16, "a": 16, "b": 16, "c": -16 },
   { "_id": 17, "a": 17, "b": 17, "c": -17 },
   { "_id": 18, "a": 18, "b": 18, "c": -18 },
   { "_id": 19, "a": 19, "b": 19, "c": -19 },
   { "_id": 20, "a": 20, "b": 20, "c": -20 }];
   checkRec( cur, expFindResult1 );
   var cur = varCL.find( new SdbQueryOption().cond( { c: 1 } ).sort( { a: 1 } ) );
   var expFindResult1 = [{ "_id": 15, "a": 15, "b": 15, "c": 1 },
   { "_id": 16, "a": 16, "b": 16, "c": 1 },
   { "_id": 17, "a": 17, "b": 17, "c": 1 },
   { "_id": 18, "a": 18, "b": 18, "c": 1 },
   { "_id": 19, "a": 19, "b": 19, "c": 1 },
   { "_id": 20, "a": 20, "b": 20, "c": 1 }];
   checkRec( cur, expFindResult1 );
}

//ָ��cond/sel/sort/hint��ѯ��������ѯ���ݸ��Ƕ���飬
//��db.foo.bar.find(new SdbQueryOption().cond().sel().sort().hint()��
//����ƥ���ֶΰ����������ͷǷ������ֶ� 
function testcombination15743 ( varCL )
{
   var cur = varCL.find( new SdbQueryOption().cond( { $and: [{ a: { $gte: 5 } }, { b: { $lte: 40 } }] } ).sel( { _id: { $include: 1 }, a: { $include: 1 }, b: { $include: 1 } } ).sort( { b: 1 } ).hint( { "": "bindex" } ) );
   var expFindResult = [{ "_id": 5, "a": 5, "b": 5 },
   { "_id": 6, "a": 6, "b": 6 },
   { "_id": 7, "a": 7, "b": 7 },
   { "_id": 8, "a": 8, "b": 8 },
   { "_id": 9, "a": 9, "b": 9 },
   { "_id": 10, "a": 10, "b": 10 },
   { "_id": 11, "a": 11, "b": 11 },
   { "_id": 12, "a": 12, "b": 12 },
   { "_id": 13, "a": 13, "b": 13 },
   { "_id": 14, "a": 14, "b": 14 },
   { "_id": 15, "a": 15, "b": 15 },
   { "_id": 16, "a": 16, "b": 16 },
   { "_id": 17, "a": 17, "b": 17 },
   { "_id": 18, "a": 18, "b": 18 },
   { "_id": 19, "a": 19, "b": 19 },
   { "_id": 20, "a": 20, "b": 20 },
   { "_id": 21, "a": 21, "b": 21 },
   { "_id": 22, "a": 22, "b": 22 },
   { "_id": 23, "a": 23, "b": 23 },
   { "_id": 24, "a": 24, "b": 24 },
   { "_id": 25, "a": 25, "b": 25 },
   { "_id": 26, "a": 26, "b": 26 },
   { "_id": 27, "a": 27, "b": 27 },
   { "_id": 28, "a": 28, "b": 28 },
   { "_id": 29, "a": 29, "b": 29 },
   { "_id": 30, "a": 30, "b": 30 },
   { "_id": 31, "a": 31, "b": 31 },
   { "_id": 32, "a": 32, "b": 32 },
   { "_id": 33, "a": 33, "b": 33 },
   { "_id": 34, "a": 34, "b": 34 },
   { "_id": 35, "a": 35, "b": 35 },
   { "_id": 36, "a": 36, "b": 36 },
   { "_id": 37, "a": 37, "b": 37 },
   { "_id": 38, "a": 38, "b": 38 },
   { "_id": 39, "a": 39, "b": 39 },
   { "_id": 40, "a": 40, "b": 40 }
   ];
   checkRec( cur, expFindResult );
}

//ָ��cond/remove��ѯ������
//��ѯ���ݸ��Ƕ���飬��db.foo.bar.find(new SdbQueryOption().cond().remove()
function testcombination15744 ( varCL )
{
   var cur = varCL.find( new SdbQueryOption().cond( { $and: [{ a: { $gte: 5 } }, { b: { $lte: 95 } }] } ).remove() );
   while( cur.next() )
   {
      var ret = cur.current();
   }
   var cur = varCL.find( new SdbQueryOption().sort( { _id: 1 } ) );
   var expFindResult = [{ "_id": 0, "a": 0, "b": 0, "c": 0 },
   { "_id": 1, "a": 1, "b": 1, "c": -1 },
   { "_id": 2, "a": 2, "b": 2, "c": -2 },
   { "_id": 3, "a": 3, "b": 3, "c": -3 },
   { "_id": 4, "a": 4, "b": 4, "c": -4 },
   { "_id": 96, "a": 96, "b": 96, "c": -96 },
   { "_id": 97, "a": 97, "b": 97, "c": -97 },
   { "_id": 98, "a": 98, "b": 98, "c": -98 },
   { "_id": 99, "a": 99, "b": 99, "c": -99 },
   { "_id": 100, "a": 100, "b": 100, "c": -100 }
   ];
   checkRec( cur, expFindResult );
}

function insertRecord ( varCL )
{
   for( var i = 0; i <= 100; i++ )
   {
      varCL.insert( { _id: i, a: i, b: i, c: -i } );
   }
}

main();