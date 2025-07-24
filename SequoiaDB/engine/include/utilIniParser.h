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

   Source File Name = utilIniParser.h

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/19/2019  HJW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTIL_INI_PARSER_H_
#define UTIL_INI_PARSER_H_

#include "core.h"

typedef struct utilIniString {
   BOOLEAN ownmem ;
   INT32 length ;
   CHAR *str ;
} utilIniString ;

typedef struct utilIniItem {
   BOOLEAN isComment ;
   utilIniString key ;
   utilIniString value ;
   utilIniString pre_comment ;
   utilIniString pos_comment ;
   struct utilIniItem *next ;
} utilIniItem ;

typedef struct utilIniSection {
   utilIniString name ;
   utilIniString comment ;
   utilIniItem *item ;
   struct utilIniSection *next ;
} utilIniSection ;

typedef struct utilIniHandler {
   INT32 errnum ;
   UINT32 flags ;
   utilIniString lastComment ;
   utilIniItem *item ;
   utilIniSection *section ;
   const CHAR *errMsg ;
} utilIniHandler ;

/* Flags */
//Section and property names are not case sensitive
#define UTIL_INI_NOTCASE            0x00000001

//Support annotation symbols ( ; )
#define UTIL_INI_SEMICOLON          0x00000002

//Support annotation symbols ( # )
#define UTIL_INI_HASHMARK           0x00000004

//Support escape character   ( '\\' )
#define UTIL_INI_ESCAPE             0x00000008

//Support Double quotation mark ( " )
#define UTIL_INI_DOUBLE_QUOMARK     0x00000010

//Support Single quotation mark ( ' )
#define UTIL_INI_SINGLE_QUOMARK     0x00000020

//Support Colon ( = )
#define UTIL_INI_EQUALSIGN          0x00000040

//Support Colon ( : )
#define UTIL_INI_COLON              0x00000080

#define UTIL_INI_FLAGS_DEFAULT (UTIL_INI_SEMICOLON|UTIL_INI_EQUALSIGN)

SDB_EXTERN_C_START

/* Common interface */
/**
 * \brief init
 * \param [in] handler ini handler
 * \param [in] flags Parse file
 */
SDB_EXPORT void utilIniInit( utilIniHandler *handler, UINT32 flags ) ;

/**
 * \brief release
 * \param [in] handler ini handler
 * \param [in] flags Parse file
 */
SDB_EXPORT void utilIniRelease( utilIniHandler *handler ) ;

/**
 * \brief Parsing ini configure
 * \param [in] handler ini handler
 * \param [in] content ini configure
 * \param [in] len content length
 */
SDB_EXPORT INT32 utilIniParse( utilIniHandler *handler, const CHAR *content ) ;

/* Get */

SDB_EXPORT INT32 utilIniGetValue( utilIniHandler *handler, const CHAR *section,
                                  const CHAR *key, const CHAR **value,
                                  INT32 *length ) ;

SDB_EXPORT INT32 utilIniGetSectionComment( utilIniHandler *handler,
                                           const CHAR *section,
                                           const CHAR **comment,
                                           INT32 *length ) ;

SDB_EXPORT INT32 utilIniGetItemPreComment( utilIniHandler *handler,
                                           const CHAR *section,
                                           const CHAR *key,
                                           const CHAR **comment,
                                           INT32 *length ) ;

SDB_EXPORT INT32 utilIniGetItemPosComment( utilIniHandler *handler,
                                           const CHAR *section,
                                           const CHAR *key,
                                           const CHAR **comment,
                                           INT32 *length ) ;

SDB_EXPORT INT32 utilIniGetLastComment( utilIniHandler *handler,
                                        const CHAR **comment,
                                        INT32 *length ) ;

/* Set */

SDB_EXPORT INT32 utilIniSetValue( utilIniHandler *handler, const CHAR *section,
                                  const CHAR *key, const CHAR *value,
                                  INT32 length ) ;

SDB_EXPORT INT32 utilIniSetSectionComment( utilIniHandler *handler,
                                           const CHAR *section,
                                           const CHAR *comment,
                                           INT32 length ) ;

SDB_EXPORT INT32 utilIniSetItemPreComment( utilIniHandler *handler,
                                           const CHAR *section,
                                           const CHAR *key,
                                           const CHAR *comment,
                                           INT32 length ) ;

SDB_EXPORT INT32 utilIniSetItemPosComment( utilIniHandler *handler,
                                           const CHAR *section,
                                           const CHAR *key,
                                           const CHAR *comment,
                                           INT32 length ) ;

SDB_EXPORT INT32 utilIniSetLastComment( utilIniHandler *handler,
                                        const CHAR *comment,
                                        INT32 length ) ;

SDB_EXPORT INT32 utilIniSetCommentItem( utilIniHandler *handler,
                                        const CHAR *section,
                                        const CHAR *key,
                                        BOOLEAN isComment ) ;

SDB_EXTERN_C_END

#endif /* UTIL_INI_PARSER_H_ */