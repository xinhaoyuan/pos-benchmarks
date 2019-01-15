#ifndef __POR_STAT_HPP__
#define __POR_STAT_HPP__

#include "Base.hpp"

#include <map>
#include <vector>

struct PorNode {
    size_t size;
    size_t minHit;
    std::map<Vertex *, size_t> index;
    std::vector<Vertex *> vertices;
    std::vector<PorNode *> children;

    PorNode() : size(0), minHit(0) { }
};

class PorTree {

    Graph * _graph;
    PorNode _root;

public:

    PorTree(Graph * g);
    ~PorTree();

    PorNode * AddPath(const std::vector<Vertex *> & path);
    inline PorNode * GetRoot() { return &_root; }
};

#endif
