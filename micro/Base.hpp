#ifndef __BASE_HPP__
#define __BASE_HPP__

#include <vector>
#include <map>
#include <random>

typedef std::mt19937_64 random_engine;

#define DBG(COND, BODY) do { if (COND) { BODY; } } while(0)

struct Vertex;

struct Edge {
    Vertex * from;
    Vertex * to;
    Edge * dualEdge; // not nullptr if the edge is undirected
    void * priv;

    void SetPriv(void * priv);
    inline bool IsDirected() { return dualEdge == nullptr; }
};

struct Vertex {
    int id;
    void * priv;
    std::vector<Edge *> inEdges;
    std::vector<Edge *> outEdges;
};

struct Graph {
    std::vector<Vertex *> vertices;
    std::vector<Edge *> edges;

    Vertex * NewVertex();
    Edge * AddEdge(Vertex * from, Vertex * to, bool directed);
    // Simple wrappers
    inline Edge * AddUndirectedEdge(Vertex * from, Vertex * to) { AddEdge(from, to, false); }
    inline Edge * AddDirectedEdge(Vertex * from, Vertex * to) { AddEdge(from, to, true); }

    ~Graph();
};

#endif
