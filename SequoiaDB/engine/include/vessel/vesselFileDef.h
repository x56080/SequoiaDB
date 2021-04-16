/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Source File Name = vesselFileDef.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_VESSEL_FILE_DEF_H_
#define VESSEL_VESSEL_FILE_DEF_H_

#include "ossTypes.h"

namespace engine
{
namespace vessel
{
   static const UINT32 MAX_FILE_NAME_LEN = 63;
   static const UINT32 MAX_SPACE_DIR_LEN = 15;

   static const CHAR * const FILE_NAME_PREFIX = "_vessel";
   static const UINT32 FILE_NAME_PREFIX_LEN = 7;

   static const CHAR * const FILE_TYPE_SUFFIX_DATAM = "dm";
   static const CHAR * const FILE_TYPE_SUFFIX_DATAD = "dd";
   static const CHAR * const FILE_TYPE_SUFFIX_IDXM = "idxm";
   static const CHAR * const FILE_TYPE_SUFFIX_IDXD = "idxd";
   static const CHAR * const FILE_TYPE_SUFFIX_LOBM = "lobm";
   static const CHAR * const FILE_TYPE_SUFFIX_LOBDM = "lobdm";
   static const CHAR * const FILE_TYPE_SUFFIX_LOBDD = "lobdd";
   static const CHAR * const FILE_TYPE_SUFFIX_FSM = "fsm";
   static const CHAR * const FILE_TYPE_SUFFIX_CSNAME = "csname";
   static const CHAR * const FILE_TYPE_SUFFIX_CONTROL = "control";
   static const CHAR * const FILE_TYPE_SUFFIX_DELTA = "delta";

   extern const CHAR * const FILE_TYPE_SUFFIX_ARRAY[];

   typedef UINT8 FILE_TYPE;
   const FILE_TYPE INVALID_FILE_TYPE = 255;
   const FILE_TYPE FILE_TYPE_DM = 0;
   const FILE_TYPE FILE_TYPE_DD = 1;
   const FILE_TYPE FILE_TYPE_IDX_M = 2;
   const FILE_TYPE FILE_TYPE_IDX_D = 3;
   const FILE_TYPE FILE_TYPE_LOB_M = 4;
   const FILE_TYPE FILE_TYPE_LOB_DM = 5;
   const FILE_TYPE FILE_TYPE_LOB_DD = 6;
   const FILE_TYPE FILE_TYPE_FSM = 7;
   const FILE_TYPE FILE_TYPE_CS_NAME = 8;
   const FILE_TYPE FILE_TYPE_CONTROL = 9;
   const FILE_TYPE FILE_TYPE_DELTA = 10;
   const FILE_TYPE FILE_TYPE_MAX = FILE_TYPE_DELTA;

   static const UINT32 FILE_TYPE_SUFFIX_ARR_SIZE = FILE_TYPE_MAX + 1;

   /// space dir name: _vessel_<space id>
   /// file name: _vessel.<space id>.<file type suffix>.<sequence>
   const UINT32 FILE_NAME_FORMAT_COLUMN_COUNT = 4;

   BOOLEAN parseFileSuffix(const CHAR *suffix, FILE_TYPE &type);
}
}

#endif//VESSEL_VESSEL_FILE_DEF_H_