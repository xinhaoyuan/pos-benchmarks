#include "Base.hpp"

using namespace std;

void Edge::SetPriv(void * priv) {
    this->priv = priv;
    if (dualEdge != nullptr) {
        this->dualEdge->priv = priv;
    }
}

Vertex * Graph::NewVertex() {
    Vertex * r = new Vertex();
    r->id = vertices.size();
    vertices.push_back(r);
    return r;
}

Edge * Graph::AddEdge(Vertex * from, Vertex * to, bool directed) {
    Edge * e = new Edge();
    e->from = from;
    e->to = to;
    from->outEdges.push_back(e);
    to->inEdges.push_back(e);
    edges.push_back(e);

    if (directed) {
        e->dualEdge = nullptr;
    }
    else {
        e->dualEdge = AddEdge(to, from, true);
        e->dualEdge->dualEdge = e;
    }

    return e;
}

Graph::~Graph() {
    for (auto v : vertices) { delete v; }
    for (auto e : edges) { delete e; }
}
