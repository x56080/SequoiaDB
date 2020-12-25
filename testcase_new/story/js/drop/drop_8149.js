/******************************************************************************
*@Description : seqDB-8149:dropCS参数校验(1)
               2. 删除名称以 $ 开头的CS 
               3. 删除名称中包含 . 的CS
               4. 删除名称为空的CS
               5. 删除名称长度为 128 字节的CS
               6. dropCS,检查listCollectionSpaces结果是否正确
               7. 再次删除已经删除的CS
*@author:      liyuanyue
*@createdate:  2020.07.17
******************************************************************************/
testConf.csName = COMMCLNAME + "_8149";
// SEQUOIADBMAINSTREAM-6212
//main( test );

function test ()
{
   var csName = COMMCLNAME + "_8149";
   var csName1 = COMMCSNAME + "_8149_1";
   var csName2 = COMMCSNAME + "_8149_2";

   // 删除名称以 $ 开头的CS
   var name = "$" + csName;
   var errno = -6;
   var message = "error,dropCS start $ cs succeeded";
   illegaldropCS( name, errno, message );

   // 删除名称中包含 . 的CS
   var name = csName + "." + csName;
   var errno = -6;
   var message = "error,dropCS contain . cs succeeded";
   illegaldropCS( name, errno, message );

   // 删除名称为空的CS
   var name = "";
   var errno = -6;
   var message = "error,dropCS empty cs succeeded";
   illegaldropCS( name, errno, message );

   // 删除名称长度为 128 字节的CS
   var name = new Array( 129 ).join( 'c' );
   var errno = -6;
   var message = "error,dropCS 128 byte cs succeeded";
   illegaldropCS( name, errno, message );

   // dropCS, 检查list结果是否正确
   db.dropCS( csName );
   var cond = { Name: csName };
   var cur = db.list( 5, cond );
   if( cur.size() != 0 )
   {
      throw new Error( "have droped the cs:" + csName + ", but it still exist" );
   }

   // 再次删除已经删除的CS
   var name = csName;
   var errno = -34;
   var message = "have droped the cs:" + csName + ", but it can be dropCS again";
   illegaldropCS( name, errno, message );
}

function illegaldropCS ( csName, errno, message )
{
   try
   {
      db.dropCS( csName );
      throw new Error( message );
   } catch( e )
   {
      if( e.message != errno )
      {
         throw e;
      }
   }
}

