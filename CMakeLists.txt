cmake_minimum_required(VERSION 3.3 FATAL_ERROR)
project(EventDisplay)
set(CMAKE_CXX_STANDARD 14)

find_package(ROOT REQUIRED COMPONENTS RIO Net Hist Graf Graf3d Gpad)

# 包含ROOT头文件和库路径
include_directories(${ROOT_INCLUDE_DIRS})
link_directories(${ROOT_LIBRARY_DIR})

# 生成字典文件
ROOT_GENERATE_DICTIONARY(${CMAKE_CURRENT_SOURCE_DIR}/src/G__EventDisplay ${CMAKE_CURRENT_SOURCE_DIR}/src/EventDisplay.hxx LINKDEF ${CMAKE_CURRENT_SOURCE_DIR}/src/EventDisplay_LinkDef.h)

# 添加源文件
set(SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/EventDisplay.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/src/IOData.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/src/CDCHit.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/src/CDCGeom.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/src/CDCLineCandidate.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/src/G__EventDisplay.cxx
)

# 创建共享库
add_library(DisplayLib SHARED ${SOURCES})

# 设置库的包含目录
target_include_directories(DisplayLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

# 链接ROOT库
target_link_libraries(DisplayLib ${ROOT_LIBRARIES} ROOT::Gui)

# 创建可执行文件
add_executable(EventDisplay ${CMAKE_CURRENT_SOURCE_DIR}/app/main.cxx)
target_link_libraries(EventDisplay PUBLIC DisplayLib)

# 安装可执行文件
install(TARGETS EventDisplay
        DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/../install)

