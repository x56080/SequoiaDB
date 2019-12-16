/* *****************************************************************************
@discretion: rename cs
             seqDB-16108 cs��domain�в��зֵ�����飬�޸�cs��
             seqDB-16109 cs��domian�У��޸�cs��ִ�������
@author��2018-10-13 chensiqin  Init
***************************************************************************** */

main( db );

function main ( db )
{
   if( commIsStandalone( db ) )
   {
      return;
   }
   if( commGetGroupsNum( db ) < 2 )
   {
      return;
   }
   var domainName = "domain16108";
   var csName1 = CHANGEDPREFIX + "_rename16108_1";
   var csName2 = CHANGEDPREFIX + "_rename16108_2";
   var csName3 = CHANGEDPREFIX + "_rename16108_3";
   var clName = CHANGEDPREFIX + "rename16108";

   //����cs cl
   commDropCS( db, csName1, true, "ignoreNotExist is true" );
   commDropCS( db, csName2, true, "ignoreNotExist is true" );
   commDropCS( db, csName3, true, "ignoreNotExist is true" );
   commDropDomain( db, domainName );

   var groups = commGetGroups( db );
   var groupName1 = groups[0][0].GroupName;
   var groupName2 = groups[1][0].GroupName;
   var domain = commCreateDomain( db, domainName, [groupName1, groupName2], { AutoSplit: true } );
   var varCS = commCreateCS( db, csName1, true, "create CS", { Domain: domainName } );
   var varCL = commCreateCL( db, csName1, clName, {}, true, false, "create cl in the beginning" )
   insertData( varCL, 100 );

   //�޸�cs���ƣ����ռ��
   testRenameCS16108( db, csName1, csName2 );
   testRenameCS16109( db, domain, csName2, csName3, clName );
   afterClear( db, domainName, csName3 );
}

function testRenameCS16108 ( db, csName1, csName2 )
{
   var oldName = csName1;
   var newName = csName2;
   db.renameCS( oldName, newName );
   checkRenameCSResult( oldName, newName, 1 );
}
/*
  �޸�cs�����鿴domain.listCollectionSpaces() 3����cs�Ƴ��� 4������� 
*/
function testRenameCS16109 ( db, domain, csName2, csName3, clName )
{
   var oldName = csName2;
   var newName = csName3;
   db.renameCS( oldName, newName );
   var csList = domain.listCollectionSpaces().toArray();
   var cs = eval( '(' + csList[0] + ')' );
   if( cs["Name"] !== csName3 )
   {
      throw buildException( "check domain.listCollectionSpaces() fail", "fail", "check", "success", "fail" );
   }
   checkDatas( csName3, clName );
}

function checkDatas ( csName3, clName )
{
   try
   {
      //check the record nums      
      var dbcl = db.getCS( csName3 ).getCL( clName );
      var count = dbcl.count();
      if( count != 100 )
      {
         throw buildException( "check datas", null, "check the new cl record nums",
            100, count );
      }

   }
   catch( e )
   {
      throw buildException( "checkDatas", e )
   }
}

function afterClear ( db, domainName, csName3 )
{
   commDropCS( db, csName3, true, "ignoreNotExist is true" );
   try 
   {
      commDropDomain( db, domainName );
   }
   catch( e )
   {
      throw buildException( "dropDomain fail", e, "drop", "success", e );
   }
}

