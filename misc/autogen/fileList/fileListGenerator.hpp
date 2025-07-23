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

   Source File Name = fileListGenerator.hpp

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef FILELIST_GENERATOR_HPP
#define FILELIST_GENERATOR_HPP

#include "../generateInterface.hpp"

/* =============== statement =============== */
#define FILENAME_LST_STATEMENT "/* This list file is automatically generated, you MUST NOT modify this file anyway! */"

#define FILENAME_SUFFIX_LIST \
   "hpp",   \
   "cpp",   \
   "h",     \
   "c",     \

#define FILENAME_FILTER_DIR   ".svn"

#define FILENAMES_LST_PATH    MISC_PATH"filenames.lst"
#define FILENAMES_HPP_PATH    ENGINE_PATH"include/filenames.hpp"

class fileListGenerator : public generateBase
{
DECLARE_GENERATOR_AUTO_REGISTER() ;

public:
   fileListGenerator() ;
   ~fileListGenerator() ;
   int init() ;
   bool hasNext() ;
   int outputFile( int id, fileOutStream &fout,
                   string &outputPath ) ;
   const char* name() { return "file list" ; }

private:
   int _genFileNameLst( fileOutStream &fout, string &outputPath ) ;
   int _genFileNameHPP( fileOutStream &fout, string &outputPath ) ;

private:
   bool _isFinish ;
   vector<string> _fileNameList ;
} ;

#endif
