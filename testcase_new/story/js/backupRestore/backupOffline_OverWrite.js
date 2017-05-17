/*******************************************************************************
@Description : 1.Test backupOffline, specify["OverWrite"]
@Modify list :
               2014-6-20  xiaojun Hu  Init
*******************************************************************************/

function main( db )
{
   var alreadStart1 = false ;
   var alreadStart2 = false ;
   var alreadStart3 = false ;
   var path = "" ;
   commDropCL( db, csName, COMMCLNAME, true, true, "Drop CL in the beginning" ) ;
   var cl = commCreateCL( db, COMMCSNAME, COMMCLNAME, -1, true, true, false,
                          "Create collection in the beginning" ) ;
   bakInsertData( cl ) ;
   bakRemoveBackups( db, CHANGEDPREFIX, true ) ;
   println( "Clear the backup in the beginning" ) ;
   // Backup specify the GroupName/Path/Description
   var runMode = commIsStandalone( db ) ;
   // In standalone,GroupName can be specified all kinds.{"GroupName":""/"abcde"}
   var groups = new Array() ;
   groups = commGetGroups( db ) ;
   if( false == runMode )
   {
      for( var i = 0 ; i < groups.length ; ++i )
      {
         println( "Begin to backup group = [" + groups[i][0].GroupName + "]" ) ;
         var bakName = COMMCLNAME + "BAK" + i ;
         var bakString = "{\"GroupName\":\"" + groups[i][0].GroupName + "\"," +
                         "\"Description\":\"backup description\"," +
                         "\"Name\":\"" + bakName + "\"," +
                         "\"EnsureInc\":false,\"OverWrite\":false}" ;
         var backup = eval("(" + bakString + ")") ;
         try
         {
            bakBackup( db, backup ) ;
            println( "Backup offline first" ) ;
         }
         catch( e )
         {
            if( -67 == e || -264 == e )
            {
               println( "backup description is already started" ) ;
               alreadStart1 = true ;
            }
            else
            {
               println( "failed to backup by specify description, rc = " + e ) ;
               throw e ;
            }
         }

         var bakString1 = "{\"GroupName\":\"" + groups[i][0].GroupName +
                          "\"," + "\"Description\":\"BAK Description\","+
                          "\"Name\":\"" + bakName + "\"," +
                          "\"EnsureInc\":false,\"OverWrite\":false}" ;
         var backup1 = eval("(" + bakString1 + ")") ;
         try
         {
            bakBackup( db, backup1 ) ;
            println( "Backup offline second" ) ;
         }
         catch( e )
         {
            if( -67 == e || -264 == e )
            {
               //println( "backup description is already started" ) ;
               alreadStart2 = false ;
               
            }
            else
            {
               println( "failed to backup by specify description, rc = " + e ) ;
               throw e ;
            }
         }

         var bakString2 = "{\"GroupName\":\""+groups[i][0].GroupName +
                          "\"," + "\"Description\":\"BAK Description\","+
                          "\"Name\":\"" + bakName + "\"," +
                          "\"EnsureInc\":false,\"OverWrite\":true}" ;
         var backup2 = eval("(" + bakString2 + ")") ;
         println( "string : " + bakString2 ) ;
         try
         {
            bakBackup( db, backup2 ) ;
            println( "Backup offline third" ) ;
         }
         catch( e )
         {
            if( -67 == e || -264 == e )
            {
               println( "backup description is already started" ) ;
               alreadStart3 = true ;
            }
            else
            {
               println( "failed to backup by specify description, rc = " + e ) ;
               throw e ;
            }
         }
         if( false == alreadStart1 && false == alreadStart2 &&
             false == alreadStart3 )
         {
            var backups = commGetBackups( db, bakName, path,
                                          alreadStart3 ) ;
            println( "backup file = " + backups ) ;
            if( 1 != backups.length )
            {
               println( "check description backup failed" ) ;
               commPrint( backups ) ;
               throw "check description backup failed" ;
            }
            println( "check backup success" ) ;
         }
      }
      bakRemoveBackups( db, bakName, alreadStart3, path ) ;
      println( "Backup Over in group" ) ;
   }
   else
   {
      var bakName = COMMCLNAME + "BAKstandalone" ;
      var bakString = "{\"Description\":\"backup description\"," +
                      "\"Name\":\"" + bakName +
                      "\",\"EnsureInc\":false,\"OverWrite\":false}" ;
      var backup = eval("(" + bakString + ")") ;
      try
      {
         bakBackup( db, backup ) ;
         println( "Backup offline first" ) ;
      }
      catch( e )
      {
         if( -67 == e || -264 == e )
         {
            println( "backup description is already started" ) ;
            alreadStart1 = true ;
         }
         else
         {
            println( "failed to backup by specify description, rc = " + e ) ;
            throw e ;
         }
      }

      var bakString1 = "{\"Description\":\"BAK Description\"," +
                       "\"Name\":\"" + bakName +
                       "\",\"EnsureInc\":false,\"OverWrite\":false}" ;
      var backup1 = eval("(" + bakString1 + ")") ;
      try
      {
         bakBackup( db, backup1 ) ;
      }
      catch( e )
      {
         if( -67 == e || -264 == e )
         {
            //println( "backup description is already started" ) ;
            alreadStart2 = false ;
            println( "Backup offline second" ) ;
         }
         else
         {
            println( "failed to backup by specify description, rc = " + e ) ;
            throw e ;
         }
      }

      var bakString2 = "{\"Description\":\"BAK Description\"," +
                       "\"Name\":\"" + bakName +
                       "\",\"EnsureInc\":false,\"OverWrite\":true}" ;
      var backup2 = eval("(" + bakString2 + ")") ;
      println( "string : " + bakString2 ) ;
      try
      {
         bakBackup( db, backup2 ) ;
         println( "Backup offline second" ) ;
      }
      catch( e )
      {
         if( -67 == e || -264 == e )
         {
            println( "backup description is already started" ) ;
            alreadStart3 = true ;
         }
         else
         {
            println( "failed to backup by specify description, rc = " + e ) ;
            throw e ;
         }
      }
      if( false == alreadStart1 && false == alreadStart2 &&
          false == alreadStart3 )
      {
         var backups = commGetBackups( db, bakName, path,
                                       alreadStart3 ) ;
         println( "backup file = " + backups ) ;
         if( 1 != backups.length )
         {
            println( "check description backup failed" ) ;
            commPrint( backups ) ;
            throw "check description backup failed" ;
         }
      }
      println( "Backup offline third" ) ;
      bakRemoveBackups( db, bakName, alreadStart3, path ) ;
   }

   println( "Clear backup Over in the end" ) ;
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, false, "Drop CL in the end" ) ;
}

try
{
   main( db ) ;
   db.close() ;
}
catch( e )
{
   throw e ;
}

