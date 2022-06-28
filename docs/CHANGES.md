## What's new on version 1.1
- Classes refactoring done.
    - ocr_tabs
- New classes added.
- API class created for a more simple, abstract way on extracting tables from document. 

### New Code Structure
```
								**module**
									*
									*
        		*****************************************
		     	*					*					*
				*					*					*
			  [CORE]			[HELPERS]			  debug 
			 document			auxiliary			dll_config
		 ocr_tabs_engine	   file_handler 	   ocr_tabs_api
		  img_processor		  drawing_handler

```

## Simple API Usage
```
#include <iostream>
#include "ocr_tabs_api.h"

int main(int argc, char *argv[]) {
	std::string filename = "test files/001.png";
	ocrt::OCRTabsAPI tabs_api(filename);
	if (tabs_api.ExtractTables()) //extracts tables and creates html file. 
		return 0;
	else 
		return 1;
}
```