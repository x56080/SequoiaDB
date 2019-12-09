/************************************
*@Description: ����������domain����AutoSplitΪtrue���޸�shardingKey���޸�domain������
*@author:      wangkexin
*@createdate:  2019.5.24
*@testlinkCase:seqDB-18351
**************************************/

main();
function main ()
{
   if( commIsStandalone( db ) )
   {
      println( "Run mode is standalone" );
      return;
   }
   //less two groups to split
   var allGroupName = getGroupName( db, true );
   if( allGroupName.length < 2 )
   {
      println( "--least two groups" );
      return;
   }

   var csName = CHANGEDPREFIX + "_cs_18351";
   var clName1 = CHANGEDPREFIX + "_cl_18351a";
   var clName2 = CHANGEDPREFIX + "_cl_18351b";
   var domainName = "domain18351";

   //clean environment before test
   commDropCS( db, csName, true, "drop CS in the beginning." );
   commDropDomain( db, domainName );

   var group1 = allGroupName[0];
   var group2 = allGroupName[1];
   commCreateDomain( db, domainName, [group1], { AutoSplit: true } );
   db.createCS( csName, { Domain: domainName } );

   var subOption = { a: 1 };
   var optionObj = { ShardingKey: subOption };
   var cl_1 = commCreateCLByOption( db, csName, clName1, optionObj );
   var cl_2 = commCreateCLByOption( db, csName, clName2, optionObj );

   //�޸�domain���ԣ�domain�������飬��������group2
   db.getDomain( domainName ).alter( { Groups: [group1, group2] } );

   //test a: �޸�shardingKey���ԣ���alter�޸�shardingKeyΪ{b��1}
   var shardingKey = { b: 1 };
   cl_1.alter( { ShardingKey: shardingKey } );
   checkAlterResult( clName1, "ShardingKey", shardingKey, csName );

   //test b: �޸�shardingKey��shardingType���ԣ�����ָ��shardingTypeΪrange
   var shardingKey2 = { b: 1 };
   var shardingType2 = "range";
   cl_2.alter( { ShardingKey: shardingKey2, ShardingType: shardingType2 } );
   checkAlterResult( clName2, "ShardingKey", shardingKey2, csName );
   checkAlterResult( clName2, "ShardingType", shardingType2, csName );

   insertData( cl_1, 5000 );
   insertData( cl_2, 5000 );

   checkSplitResult( csName, clName1, group1, group2, 5000 );
   checkNotSplitResult( csName, clName2, group1, group2, 5000 );

   //clean
   commDropCS( db, csName, true, "clean cs" );
   commDropDomain( db, domainName );
}
