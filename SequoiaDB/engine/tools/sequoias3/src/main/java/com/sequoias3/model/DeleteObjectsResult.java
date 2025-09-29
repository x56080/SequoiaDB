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

   Source File Name = DeleteObjectsResult.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoias3.model;

import com.fasterxml.jackson.annotation.JsonInclude;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.dataformat.xml.annotation.JacksonXmlElementWrapper;
import com.fasterxml.jackson.dataformat.xml.annotation.JacksonXmlRootElement;

import java.util.ArrayList;
import java.util.List;

@JacksonXmlRootElement(localName = "DeleteResult")
public class DeleteObjectsResult {
    @JacksonXmlElementWrapper(localName = "Deleted", useWrapping = false)
    @JsonProperty("Deleted")
    private List<ObjectDeleted> deletedObjects;

    @JacksonXmlElementWrapper(localName = "Error", useWrapping = false)
    @JsonProperty("Error")
    private List<DeleteError> errors;

    public DeleteObjectsResult(){
        deletedObjects = new ArrayList<ObjectDeleted>();
        errors = new ArrayList<DeleteError>();
    }

    public void setDeletedObjects(List<ObjectDeleted> deletedObjects) {
        this.deletedObjects = deletedObjects;
    }

    public List<ObjectDeleted> getDeletedObjects() {
        return deletedObjects;
    }

    public void setErrors(List<DeleteError> errors) {
        this.errors = errors;
    }

    public List<DeleteError> getErrors() {
        return errors;
    }
}
