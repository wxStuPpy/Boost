CXX=g++

CXXFLAGS=-Wall  -std=c++14

AsyncSession:AsyncSession.cpp
	-${CXX} $^ ${CXXFLAGS} -o $@
	-./$@ 
	-rm ./$@

SyncClient:SyncClient.cpp
	-${CXX} $^ ${CXXFLAGS} -o $@
	-./$@ 
	-rm ./$@

myAsync:myAsync.cpp
	-${CXX} $^ ${CXXFLAGS} -o $@
	-./$@ 
	-rm ./$@

