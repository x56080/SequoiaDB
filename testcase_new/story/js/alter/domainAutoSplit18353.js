/************************************
*@Description: ����������domain����AutoSplitΪtrue���޸�shardingKey��shardingType
*@author:      wangkexin
*@createdate:  2019.5.24
*@testlinkCase:seqDB-18353
**************************************/

try
{
   main();
}
catch( e )
{
   if( e.constructor === Error )
   {
      println( e.stack );
   }
   throw e;
}

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

   var csName = CHANGEDPREFIX + "_cs_18353";
   var clName1 = CHANGEDPREFIX + "_cl_18353a";
   var clName2 = CHANGEDPREFIX + "_cl_18353b";
   var domainName = "domain18353";

   //clean environment before test
   commDropCS( db, csName, true, "drop CS in the beginning." );
   commDropDomain( db, domainName );

   var group1 = allGroupName[0];
   var group2 = allGroupName[1];
   commCreateDomain( db, domainName, [group1], { AutoSplit: true } );
   db.createCS( csName, { Domain: domainName } );

   var subOption1 = { a: 1 };
   var optionObj1 = { ShardingKey: subOption1, ShardingType: "hash" };
   var cl_1 = commCreateCL( db, csName, clName1, optionObj1 );

   var subOption2 = { a: 1 };
   var optionObj2 = { ShardingKey: subOption2, ShardingType: "range" };
   var cl_2 = commCreateCL( db, csName, clName2, optionObj2 );

   //�޸�domain���ԣ�domain�������飬��������group2
   db.getDomain( domainName ).alter( { Groups: [group1, group2] } );

   //test a: ������a�޸�shardingKey��shardingType��ָ��shardingTypeΪrange
   var shardingKey = { b: 1 };
   var shardingType = "range";
   cl_1.alter( { ShardingKey: shardingKey, ShardingType: shardingType } );
   checkAlterResult( clName1, "ShardingKey", shardingKey, csName );
   checkAlterResult( clName1, "ShardingType", shardingType, csName );

   //test b: ������b�޸�shardingKey��shardingType��ָ��shardingTypeΪhash
   var shardingKey2 = { b: 1 };
   var shardingType2 = "hash";
   cl_2.alter( { ShardingKey: shardingKey2, ShardingType: shardingType2 } );
   checkAlterResult( clName2, "ShardingKey", shardingKey2, csName );
   checkAlterResult( clName2, "ShardingType", shardingType2, csName );

   insertData( cl_1, 5000 );
   insertData( cl_2, 5000 );

   checkNotSplitResult( csName, clName1, group1, group2, 5000 );
   checkSplitResult( csName, clName2, group1, group2, 5000 );

   //clean
   commDropCS( db, csName, true, "clean cs" );
   commDropDomain( db, domainName );
}
