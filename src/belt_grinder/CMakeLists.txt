cmake_minimum_required(VERSION 2.8.3)
project(belt_grinder)

find_package(catkin REQUIRED COMPONENTS roscpp)
find_package(ethercat_manager minas_control ATI_sensor compensation_cylinder)

###################################
## catkin specific configuration ##
###################################
catkin_package(
  INCLUDE_DIRS include
  CATKIN_DEPENDS roscpp
##  LIBRARIES ATI_sensor
)

###########
## Build ##
###########

include_directories(
  include
  ${catkin_INCLUDE_DIRS}
)

## Declare a C++ library
## add_library(ATI_sensor
##   src/ATI_sensor.cpp
## )

## Specify libraries to link a library or executable target against
## add_executable(belt_grinder src/belt_grinder.cpp)
target_link_libraries(belt_grinder ATI_sensor compensation_cylinder minas_control ${catkin_LIBRARIES} ${Boost_LIBRARIES})

#############
## Install ##
#############

# Mark executables and/or libraries for installation
install(TARGETS ATI_sensor
  ARCHIVE DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
  LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
  RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
)

install(DIRECTORY include/${PROJECT_NAME}/
  DESTINATION ${CATKIN_PACKAGE_INCLUDE_DESTINATION})


#############
## Testing ##
#############

if(CATKIN_ENABLE_TESTING)
  find_package(catkin REQUIRED COMPONENTS rostest roslaunch)
  file(GLOB LAUNCH_FILES launch/*.launch)
  foreach(LAUNCH_FILE ${LAUNCH_FILES})
    message(status "Testing ${LAUNCH_FILE}")
    roslaunch_add_file_check(${LAUNCH_FILE})
  endforeach()
endif()

#############
## Release ##
#############

# debbuild
# catkin build minas_control --make-args debbuild
# ROS_PARALLEL_JOBS='' catkin_make --pkg minas_control -- debbuild
add_custom_target(debbuild_${PROJECT_NAME}
  COMMAND bloom-generate rosdebian --os-name ubuntu --os-version trusty --ros-distro indigo
  COMMAND MAKEFLAGS= fakeroot debian/rules binary
  COMMAND rm -fr debian obj-x86_64-linux-gnu
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  )
