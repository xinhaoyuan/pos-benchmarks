#ifndef __GENERATORS_HPP__
#define __GENERATORS_HPP__

#include "Base.hpp"
#include <vector>
#include <map>
#include <utility>

namespace Generator {
    void RainbowSkeleton(Graph * g, int width, int length);
    void DoubleTreeSkeleton(Graph * g, int depth);
    void AntiChain(Graph * g, int size);
    void Star(Graph * g, int fanout);
    void AddUniformPairDependency(Graph * g, random_engine & random, double density);
    void AddRWDependency(Graph * g, random_engine & random, double idleRatio, double rwRatio, double skew, int total, std::map<int, std::tuple<int, bool>> & rwInfo);
    void AddRWDistDependency(Graph * g, random_engine & engine, const std::vector<int> & rwDist, std::map<int, std::tuple<int, bool>> & rwInfo);

    void RandomTree(Graph * g, random_engine & random);
    void RandomDag(Graph * g, random_engine & random, int leave);
}

#endif
