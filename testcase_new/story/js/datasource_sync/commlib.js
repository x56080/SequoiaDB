/******************************************************************************
 * @Description   : 
 * @Author        : liuli
 * @CreateTime    : 2021.02.04
 * @LastEditTime  : 2021.03.01
 * @LastEditors   : liuli
 ******************************************************************************/
import( "../lib/datasource_commlib.js" );

function createUsrAndPasswd ( db )
{
   try
   {
      db.createUsr( userName, passwd );
   }
   catch( e )
   {
      if( e != SDB_AUTH_USER_ALREADY_EXIST )
      {
         throw e;
      }
   }
}
