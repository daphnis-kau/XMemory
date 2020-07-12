include_directories ("${PROJECT_SOURCE_DIR}/include")
set(ProjectName core)

if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

set(export_files
	${PROJECT_SOURCE_DIR}/include/Memory.h
	${PROJECT_SOURCE_DIR}/include/New.h)
source_group("export" FILES ${export_files})

set(head_files
	Misc.h
	TList.h
	LockHelp.h
	CMemoryMgr.h
	CDebug.h)
source_group("include" FILES ${head_files})

set(source_files
	CDebug.cpp 
	CMemoryMgr.cpp 
	Memory.cpp)	
source_group("source" FILES ${source_files})

add_library(
	${ProjectName} 
	${export_files} 
	${head_files} 
	${source_files})	