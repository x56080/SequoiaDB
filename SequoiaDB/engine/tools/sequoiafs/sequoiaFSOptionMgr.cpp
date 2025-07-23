/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = sequoiaFSOptionMgr.cpp

   Descriptive Name = sequoiafs options manager.

   When/how to use:  This program is used on sequoiafs.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date      Who Description
   ====== =========== === ==============================================
      03/05/2018  YWX  Initial Draft

   Last Changed =

*******************************************************************************/
#include "sequoiaFSOptionMgr.hpp"
#include "pmdOptionsMgr.hpp"
#include "pmdDef.hpp"
#include "ossVer.hpp"

using namespace engine;
using namespace sequoiafs;

static size_t name_hash( const char *name )
{
   uint64_t hash = 5381;

   for ( ; *name; name++ )
   {
     hash = hash * 31 + ( unsigned char ) *name;
   }
   return hash;
}

INT32 _sequoiafsOptionMgr::parseCollection( const string collection,
                                     string *cs,
                                     string *cl )
{
   INT32 rc = SDB_OK;
   string clFullName;
   size_t is_index = 0;

   clFullName = collection;

   is_index = clFullName.find( '.' );
   if( is_index != std::string::npos )
   {
     *cl = clFullName.substr( is_index + 1 );
     *cs = clFullName.substr( 0, is_index );
   }

   else
   {
     rc = SDB_INVALIDARG;
     ossPrintf( "The input collection's pattern is wrong( error=%d ), "
                "collecion:%s, exit."OSS_NEWLINE, rc, clFullName.c_str() );
     goto error;
   }

done:
   return rc;
error:
   goto done;

}


INT32 _sequoiafsOptionMgr::save()
{
   INT32 rc = SDB_OK ;
   std::string line ;

   if( ossStrcmp( _cfgPath, "" ) != 0 )
   {
      rc = pmdCfgRecord::toString(  line, PMD_CFG_MASK_SKIP_UNFIELD  ) ;
      if (  SDB_OK != rc  )
      {
         PD_LOG(  PDERROR, "Failed to get the line str:%d", rc  ) ;
         goto error ;
      }

      rc = utilWriteConfigFile(  _cfgFileName, line.c_str() , FALSE  ) ;
      PD_RC_CHECK(  rc, PDERROR, "Failed to write config[%s], rc: %d",
                  _cfgFileName, rc  ) ;
   }

done:
   return rc ;
error:
   goto done ;
}

_sequoiafsOptionMgr::_sequoiafsOptionMgr()
{
   ossMemset( _hosts, 0, sizeof( _hosts ) );
   ossMemset( _userName, 0, sizeof( _userName ) );
   ossMemset( _passwd, 0, sizeof( _passwd ) );
   ossMemset( _collection, 0, sizeof( _collection ) );
   ossMemset( _metaFileCollection, 0, sizeof( _metaFileCollection ) );
   ossMemset( _metaDirCollection, 0, sizeof( _metaDirCollection ) );
   ossMemset( _cfgPath, 0, sizeof( _cfgPath ) );
   ossMemset( _cfgFileName, 0, sizeof( _cfgFileName ) );
   ossMemset( _diagPath, 0, sizeof( _diagPath ) );

   _connectionNum = SDB_SEQUOIAFS_CONNECTION_DEFAULT_MAX_NUM;
   _cacheSize = SDB_SEQUOIAFS_CACHE_DEFAULT_SIZE;
   _diagLevel = PDWARNING;
   _hasOptionAutocreate = FALSE;
}

PDLEVEL _sequoiafsOptionMgr::getDiaglogLevel()const
{
   PDLEVEL level = PDWARNING;

   if( _diagLevel < PDSEVERE )
   {
     level = PDSEVERE;
   }
   else if( _diagLevel > PDDEBUG )
   {
     level = PDDEBUG;
   }
   else
   {
     level = ( PDLEVEL )_diagLevel;
   }

   return level;
}


INT32 _sequoiafsOptionMgr::init( INT32 argc,
                            CHAR **argv,
                            vector<string> *options4fuse )
{
   INT32 rc = SDB_OK;
   CHAR *cfgPath;
   CHAR cfgTempPath[OSS_MAX_PATHSIZE + 1] = {0};
   const CHAR *tempPath;
   UINT32 i = 0;
   stringstream ss;
   string tempStr;
   string tempCSStr;
   string tempCLStr;
   string tempDirStr;
   string tempFileStr;
   namespace po = boost::program_options;

   ossSnprintf( cfgTempPath, sizeof( cfgTempPath ),
             "Usage:%s mountpoint [options]\n\nCommand options", argv[0] );
   //1. init options
   po::options_description desc( cfgTempPath );
   po::options_description display( "Command options( display )" );
   po::variables_map vmFromCmd;
   po::variables_map vmFromFile;

   desc.add_options()
     ( PMD_COMMANDS_STRING( SDB_SEQUOIAFS_HELP,           ",h" ),
       "Print help message" )
     ( SDB_SEQUOIAFS_HELP_FUSE,
       "Print fuse help message" )
     ( PMD_COMMANDS_STRING( SDB_SEQUOIAFS_VERSION,        ",v" ),
       "Print version message" )
     ( PMD_COMMANDS_STRING( SDB_SEQUOIAFS_HOSTS,          ",i" ),
       po::value<std::string>() ,
       "host addresses( hostname:svcname ), separated by ',', such as "
       "'localhost:11810,localhost:11910', default:'localhost:11810'" )
     ( PMD_COMMANDS_STRING( SDB_SEQUOIAFS_USERNAME,       ",u" ),
       po::value<std::string>() , "user name of source sdb" )
     ( PMD_COMMANDS_STRING( SDB_SEQUOIAFS_PASSWD,         ",p" ),
       po::value<std::string>() , "user passwd of source sdb" )
     ( PMD_COMMANDS_STRING( SDB_SEQUOIAFS_COLLECTION,     ",l" ),
       po::value<std::string>() , "the target collection that be mounted" )
     ( PMD_COMMANDS_STRING( SDB_SEQUOIAFS_META_DIR_CL,    ",d" ),
       po::value<std::string>() , "the dir meta collection, default:sequoiafs.xxx" )
     ( PMD_COMMANDS_STRING( SDB_SEQUOIAFS_META_FILE_CL,   ",f" ),
       po::value<std::string>() , "the file meta collection, default:sequoiafs.xxx" )
     ( PMD_COMMANDS_STRING( SDB_SEQUOIAFS_CONNECTION_NUM, ",n" ),
       po::value<INT32>() ,      "the max connection num of the connection pool, "
       "default:100, value range: [50-1000]" )
     ( PMD_COMMANDS_STRING( SDB_SEQUOIAFS_CACHE_SIZE,     ",s" ),
       po::value<INT32>() ,      "the cache size( unit:M ) of dir meta, "
       "default:2, value range: [1-200]" )
     ( PMD_COMMANDS_STRING( SDB_SEQUOIAFS_CONF_PATH,      ",c" ),
       po::value<std::string>() , "the path of configure file, "
       "default: ./conf/sequoiafs.conf" )
     ( PMD_COMMANDS_STRING( SDB_SEQUOIAFS_DIAGLEVEL,      ",g" ),
       po::value<INT32>() ,      "diagnostic level, default:3, value range: [0-5]" )
     ( SDB_SEQUOIAFS_DIAGNUM, po::value<INT32>() ,
       "The max number of diagnostic log files, default:20, -1:unlimited" )
     ( SDB_SEQUOIAFS_DIAGPATH, po::value<std::string>(),
       "Diagnostic log file path" )
     ( SDB_SEQUOIAFS_AUTOCREATE,"auto create collections for file and dir meta, "
       "if not specified \"-d\" or \"-f\", should add this option to auto create" )
     ;
   //2. pick up the options for fuse
   po::detail::cmdline cmd( argc, argv );
   cmd.set_options_description( desc );
   cmd.allow_unregistered();
   vector<po::option> result;
   po::option temp;
   vector<string> unregisted_str;
   try
   {
     result = cmd.run() ;
   }
   catch( po::unknown_option &e )
   {
     std::cerr << "Unkown argument:" << e.get_option_name()  << std::endl;
     rc = SDB_INVALIDARG;
     goto error;
   }

   catch(po::invalid_option_value &e)
   {
      std::cerr << "Invalid argument:" << e.get_option_name() << std::endl;
      rc = SDB_INVALIDARG;
      goto error;
   }

   catch(po::error &e)
   {
      std::cerr << e.what() << std::endl;
      rc = SDB_INVALIDARG;
      goto error;
   }

   for(i=0; i< result.size(); i++)
   {
      temp= result[i];
      if(TRUE == temp.unregistered)
      {
         if("" != temp.string_key)
         {
            unregisted_str.push_back(temp.string_key);
         }
      }
      else if("" == temp.string_key && "" != temp.value[0])
      {
          unregisted_str.push_back(temp.value[0]);
      }
   }

   //2. init configuration file
   rc = engine::utilReadCommandLine(argc, argv, desc, vmFromCmd, TRUE);
   if(SDB_OK != rc)
   {
      goto error;
   }

   if(vmFromCmd.count(SDB_SEQUOIAFS_HELP))
   {
      std::cout << desc << std::endl;
      unregisted_str.push_back("--helpsfs");
      rc = SDB_PMD_HELP_ONLY;
      goto done;
   }

   if(vmFromCmd.count(SDB_SEQUOIAFS_HELP_FUSE))
   {
      unregisted_str.push_back("--help");
      rc = SDB_PMD_HELP_ONLY;
      goto done;
   }

   if(vmFromCmd.count(SDB_SEQUOIAFS_VERSION))
   {
      ossPrintVersion("SequoiaFS version");
      unregisted_str.push_back("--version");
      rc = SDB_PMD_VERSION_ONLY;
      goto done;
   }

   if(vmFromCmd.count(SDB_SEQUOIAFS_AUTOCREATE))
   {
      _hasOptionAutocreate = TRUE;
   }

   if((vmFromCmd.count(SDB_SEQUOIAFS_META_DIR_CL) ||
       vmFromCmd.count(SDB_SEQUOIAFS_META_FILE_CL)) &&\
      _hasOptionAutocreate)
   {
      ossPrintf("If \"--autocreate\" is specified, no need to "
                "specify \"-d\" or \"-f\"."OSS_NEWLINE);
      rc = SDB_INVALIDARG;
      goto error;
   }

   tempPath = (vmFromCmd.count(SDB_SEQUOIAFS_CONF_PATH)) ?
              (vmFromCmd[SDB_SEQUOIAFS_CONF_PATH].as<string>().c_str()) :
              PMD_CURRENT_PATH;
   ossMemset(cfgTempPath, 0, sizeof(cfgTempPath));
   cfgPath = ossGetRealPath(tempPath, cfgTempPath, OSS_MAX_PATHSIZE);
   if(!cfgPath)
   {
      if(vmFromCmd.count(SDB_SEQUOIAFS_CONF_PATH))
         std::cerr << "ERROR: Failed to get real path for "<< tempPath<< endl;
      else
         SDB_ASSERT(FALSE, "Current path is impossible to be NULL");

      rc = SDB_INVALIDPATH;
      goto error;
   }

   rc = engine::utilBuildFullPath(cfgTempPath, SDB_SEQUOIAFS_CFG_FILE_NAME,
                                  OSS_MAX_PATHSIZE, _cfgFileName);
   if(SDB_OK != rc)
   {
      std::cerr << "ERROR: Make configuration file name failed, rc:" <<\
                rc << endl;
      goto error;
   }

   rc = ossAccess( _cfgFileName, OSS_MODE_READ );
   if ( SDB_OK == rc )
   {
      rc = engine::utilReadConfigureFile(_cfgFileName, desc, vmFromFile);
      if(SDB_OK != rc)
      {
         if(vmFromCmd.count(SDB_SEQUOIAFS_CONF_PATH))
         {
            std::cerr << "ERROR: Read configuration file[" << _cfgFileName <<\
                         "] failed[" << rc << "]" << endl;
            goto error;
         }
      }
   }

   rc = pmdCfgRecord::init(&vmFromFile, &vmFromCmd);
   if(SDB_OK != rc)
   {
      std::cerr << "ERROR: Init configuration record failed[" << rc << "]" << endl;
      goto error;
   }

   rc = postLoaded(engine::PMD_CFG_STEP_INIT);
   if(SDB_OK != rc)
   {
      goto error;
   }

done:
   *options4fuse = unregisted_str;
   return rc;

error:
   goto done;
}

INT32 _sequoiafsOptionMgr::doDataExchange(pmdCfgExchange *pEX)
{
   resetResult();
   //--hosts
   rdxString(pEX, SDB_SEQUOIAFS_HOSTS, _hosts, sizeof(_hosts), FALSE,
             PMD_CFG_CHANGE_FORBIDDEN, SDB_SEQUOIAFS_HOSTS_DEFAULT_VALUE);
   //--username
   rdxString(pEX, SDB_SEQUOIAFS_USERNAME, _userName, sizeof(_userName),
             FALSE, PMD_CFG_CHANGE_FORBIDDEN, SDB_SEQUOIAFS_USER_DEFAULT_NAME);
   //--passwd
   rdxString(pEX, SDB_SEQUOIAFS_PASSWD, _passwd, sizeof(_passwd), FALSE,
             PMD_CFG_CHANGE_FORBIDDEN, SDB_SEQUOIAFS_USER_DEFAULT_PASSWD);
   //--collection
   rdxString(pEX, SDB_SEQUOIAFS_COLLECTION, _collection, sizeof(_collection),
             TRUE, PMD_CFG_CHANGE_FORBIDDEN, "");
   rdvNotEmpty(pEX, _collection);
   //--metafilecollection
   rdxString(pEX, SDB_SEQUOIAFS_META_FILE_CL, _metaFileCollection,
             sizeof(_metaFileCollection), FALSE, PMD_CFG_CHANGE_FORBIDDEN, "");
   //--metadircollection
   rdxString(pEX, SDB_SEQUOIAFS_META_DIR_CL, _metaDirCollection,
             sizeof(_metaDirCollection), FALSE, PMD_CFG_CHANGE_FORBIDDEN, "");

   //--connectionnum
   rdxInt(pEX, SDB_SEQUOIAFS_CONNECTION_NUM, _connectionNum, FALSE,
          PMD_CFG_CHANGE_FORBIDDEN, SDB_SEQUOIAFS_CONNECTION_DEFAULT_MAX_NUM);
   rdvMinMax(pEX, _connectionNum, 50, 1000, TRUE);

   //--cachesize
   rdxInt(pEX, SDB_SEQUOIAFS_CACHE_SIZE, _cacheSize, FALSE,
          PMD_CFG_CHANGE_FORBIDDEN, SDB_SEQUOIAFS_CACHE_DEFAULT_SIZE);
   rdvMinMax(pEX, _cacheSize, 1, 200, TRUE);

   //--diaglevel
   rdxUShort(pEX, SDB_SEQUOIAFS_DIAGLEVEL, _diagLevel, FALSE,
             PMD_CFG_CHANGE_RUN, (UINT16)PDWARNING);
   rdvMinMax(pEX, _diagLevel, PDSEVERE, PDDEBUG, TRUE);

   //--confpath
   rdxPath(pEX, SDB_SEQUOIAFS_CONF_PATH, _cfgPath, sizeof(_cfgPath),
           FALSE, PMD_CFG_CHANGE_FORBIDDEN, "");
   //--diagpath
   rdxPath(pEX, SDB_SEQUOIAFS_DIAGPATH, _diagPath, sizeof(_diagPath),
           FALSE, PMD_CFG_CHANGE_FORBIDDEN, PMD_CURRENT_PATH);
   //--diagnum
   rdxInt(pEX, SDB_SEQUOIAFS_DIAGNUM, _diagnum, FALSE, PMD_CFG_CHANGE_RUN,
          PD_DFT_FILE_NUM);
   rdvMinMax( pEX, _diagnum, (INT32)PD_MIN_FILE_NUM, (INT32)OSS_SINT32_MAX, TRUE ) ;
   return getResult();
}

INT32 _sequoiafsOptionMgr::postLoaded( PMD_CFG_STEP step )
{
   INT32 rc = SDB_OK;
   INT64 hash = 1;
   stringstream ss;
   string tempCSStr;
   string tempCLStr;
   string tempDirStr;
   string tempFileStr;

   if(_hasOptionAutocreate)
   {
      rc = parseCollection(_collection, &tempCSStr, &tempCLStr);
      if(SDB_OK != rc)
      {
         goto error;
      }
      hash = name_hash(_collection);
      ss.str("");
      ss << hash;
      tempFileStr= SEQUOIAFS_META_CS + "." + tempCLStr.substr(0, 32) +
                   SEQUOIAFS_META_FILE_SUFFIX + ss.str();
      ossSnprintf(_metaFileCollection, sizeof(_metaFileCollection),
                  "%s", tempFileStr.c_str());
      tempDirStr = SEQUOIAFS_META_CS + "." + tempCLStr.substr(0, 32) +
                   SEQUOIAFS_META_DIR_SUFFIX + ss.str();
      ossSnprintf(_metaDirCollection, sizeof(_metaDirCollection),
                  "%s", tempDirStr.c_str());
   }
   else if(0 == ossStrcmp(_metaFileCollection, "") ||
           0 == ossStrcmp(_metaDirCollection, ""))
   {
      if(0 == ossStrcmp(_metaFileCollection, "") &&
         0 == ossStrcmp(_metaDirCollection, ""))
      {
         ossPrintf("Warning: Fields [metadircollection] and [metafilecollection] "
                   "are empty, need to specify \"--autocreate\" to "
                   "create automatically."OSS_NEWLINE);
      }
      else
      {
         ossPrintf("Warning: Field [%s] is empty."OSS_NEWLINE,
                   (0 == ossStrcmp(_metaFileCollection, ""))?
                   "metafilecollection":"metadircollection");
      }
      rc = SDB_INVALIDARG;
   }


done:
return rc ;
error:
goto done ;

}



