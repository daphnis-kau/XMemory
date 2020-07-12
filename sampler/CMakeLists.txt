include_directories (
	"${PROJECT_SOURCE_DIR}/include")

set(ProjectName sampler)

set(head_files
	sampler.h)
source_group("include" FILES ${head_files})

set(source_files
	sampler.cpp)	
source_group("source" FILES ${source_files})

add_executable(
	${ProjectName} 
	${head_files} 
	${source_files})
target_link_libraries(sampler core)