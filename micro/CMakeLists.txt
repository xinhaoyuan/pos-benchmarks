CMAKE_MINIMUM_REQUIRED(VERSION 3.1)
PROJECT(contest-sch-benchmarks-mini)

SET(CMAKE_CXX_STANDARD 11)

ADD_LIBRARY(MiniBench STATIC PorStat.cpp Schedulers.cpp Generators.cpp Base.cpp)

ADD_EXECUTABLE(Main Main.cpp)
TARGET_LINK_LIBRARIES(Main MiniBench)

ADD_EXECUTABLE(Calc Calc.cpp)
TARGET_LINK_LIBRARIES(Calc MiniBench)

ADD_EXECUTABLE(DataGen DataGen.cpp)
TARGET_LINK_LIBRARIES(DataGen MiniBench)

ADD_EXECUTABLE(TreeTraversal TreeTraversal.cpp)
TARGET_LINK_LIBRARIES(TreeTraversal MiniBench)
