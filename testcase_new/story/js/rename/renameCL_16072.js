/* *****************************************************************************
@discretion: rename cl
             seqDB-16072
@author��2018-10-15 chensiqin  Init
***************************************************************************** */
main( db );

function main ( db )
{
   if( true == commIsStandalone( db ) )
   {
      println( "run mode is standalone" );
      return;
   }
   var csName = CHANGEDPREFIX + "_cs16072";
   var clName = CHANGEDPREFIX + "_cl16072";
   var newClName = CHANGEDPREFIX + "_newcl16072";
   var pcdName = "procedure16072"

   commDropCS( db, csName, true, "drop CS " + csName );
   try
   {
      db.removeProcedure( "test16072" );
   }
   catch( e )
   {
      if( e != -233 )
      {
         throw buildException( "removeProcedure", null, "check removeProcedure exception",
            -233, e );
      }
   }
   var cs = commCreateCS( db, csName, true, "create CS1" );
   var varCL = commCreateCLByOption( db, csName, clName, {}, true, false, "create cl in the beginning" );

   var recordNums = 100;
   insertData( varCL, recordNums );

   //1.�����洢���̣�ָ��cl��ѯ
   db.createProcedure( function test16072 ( csName, clName ) { return db.getCS( csName ).getCL( clName ).count( { a: { $lt: 95 } } ); } );
   var ret = db.eval( 'test16072("' + csName + '", "' + clName + '")' );
   if( ret != 95 )
   {
      throw buildException( "check datas", null, "check the cl record nums",
         95, ret );
   }

   //2.renameCL �ٴ�ִ�д洢���̲�ѯ
   try
   {
      cs.renameCL( clName, newClName );
      ret = db.eval( 'test16072("' + csName + '", "' + clName + '")' );
   }
   catch( e )
   {
      if( e != -23 )
      {
         throw buildException( "renameCS( clName, newClName ) fail", e, "rename", "success", e );
      }
   }
   //3.�½�clָ��Ϊ�ɵ�cl���������¼���ٴ�ִ�д洢���̲�ѯ
   commCreateCLByOption( db, csName, clName, {}, true, false, "create cl in the beginning" );
   insertData( varCL, 50 );
   ret = db.eval( 'test16072("' + csName + '", "' + clName + '")' );

   if( ret != 50 )
   {
      throw buildException( "check datas", null, "check the cl record nums",
         50, ret );
   }

   db.removeProcedure( "test16072" );
   commDropCS( db, csName, true, "ignoreNotExist is true" );
}

function checkDatas ( csName, newCLName, expRecordNums )
{
   try
   {
      //check the record nums      
      var dbcl = db.getCS( csName ).getCL( newCLName );
      var count = dbcl.count();
      if( count != expRecordNums )
      {
         throw buildException( "check datas", null, "check the new cl record nums",
            expRecordNums, count );
      }
   }
   catch( e )
   {
      throw buildException( "checkDatas", e )
   }
}
