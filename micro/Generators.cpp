#include "Generators.hpp"
#include <random>
#include <iostream>
#include <cassert>
#include <algorithm>

using namespace std;

namespace Generator {

    void RainbowSkeleton(Graph * g, int width, int length) {
        for (int i = 0; i < width; ++i) {
            vector<Vertex *> chain;
            for (int j = 0; j < length; ++j) {
                auto v = g->NewVertex();

                if (chain.size() > 0) {
                    g->AddDirectedEdge(chain.back(), v);
                }

                chain.push_back(v);
            }
        }
    }

    static tuple<Vertex *, Vertex *> ConstructSubDoubleTree(Graph * g, int depth) {
        Vertex * a = g->NewVertex();
        if (depth > 0) {
            auto l = ConstructSubDoubleTree(g, depth - 1);
            auto r = ConstructSubDoubleTree(g, depth - 1);
            g->AddDirectedEdge(a, get<0>(l));
            g->AddDirectedEdge(a, get<0>(r));
            Vertex * b = g->NewVertex();
            g->AddDirectedEdge(get<1>(l), b);
            g->AddDirectedEdge(get<1>(r), b);
            return make_tuple(a, b);
        }
        else return make_tuple(a, a);
    }

    void DoubleTreeSkeleton(Graph * g, int depth) {
        ConstructSubDoubleTree(g, depth);
    }

    void AntiChain(Graph * g, int size) {
        for (int i = 0; i < size; ++i) {
            g->NewVertex();
        }
    }

    void Star(Graph * g, int fanout) {
        vector<Vertex *> fanoutVertices;
        Vertex * center = g->NewVertex();
        for (int i = 0; i < fanout; ++i) {
            fanoutVertices.push_back(g->NewVertex());
        }

        for (int i = 0; i < fanout; ++i) {
            g->AddUndirectedEdge(center, fanoutVertices[i]);
        }
    }

    void AddUniformPairDependency(Graph * g, random_engine & random, double density) {
        uniform_real_distribution<double> dist(0.0, 1.0);

        for (int i = 0; i < g->vertices.size(); ++i) {
            for (int j = i + 1; j < g->vertices.size(); ++j) {
                if (dist(random) < density) {
                    g->AddUndirectedEdge(g->vertices[i], g->vertices[j]);
                }
            }
        }
    }

    void AddRWDependency(Graph * g, random_engine & random, double idleRatio, double rwRatio, double skew, int total, map<int, tuple<int, bool>> & rwInfo) {
        uniform_real_distribution<double> dist(0.0, 1.0);
        rwInfo.clear();

        for (int i = 0; i < g->vertices.size(); ++i) {
            int id;

            if (dist(random) <= idleRatio) {
                rwInfo[i] = make_tuple(total, false);
                continue;
            }

            bool isWrite = dist(random) > rwRatio;

            if (skew > 0) {
                id = 0;
                while (true) {
                    if (dist(random) < skew)
                        break;
                    id = (id + 1) % total;
                }
            }
            else {
                uniform_int_distribution<int> udist(0, total - 1);
                id = udist(random);
            }

            for (int j = 0; j < i; ++j) {
                if (get<0>(rwInfo[j]) == id) {
                    if (get<1>(rwInfo[j]) || isWrite) {
                        g->AddUndirectedEdge(g->vertices[i], g->vertices[j]);
                    }
                }
            }

            rwInfo[i] = make_tuple(id, isWrite);
        }

        // bool first = true;
        // for (auto && kv : rwInfo) {
        //     if (first) first = false;
        //     else cout << ' ';
        //     cout << kv.first << '(' << get<0>(kv.second) << ',' << get<1>(kv.second) << ')';
        // }
        // cout << endl;
    }

    void AddRWDistDependency(Graph * g, random_engine & random, const vector<int> & rwDist, map<int, tuple<int, bool>> & rwInfo) {
        rwInfo.clear();
        vector<tuple<int, bool>> pool;

        for (int i = 0; i < rwDist.size(); ++i) {
            int objId = i / 2;
            bool isWrite = i % 2 == 0;
            for (int j = 0; j < rwDist[i]; ++j) {
                pool.push_back(make_tuple(objId, isWrite));
            }
        }

        assert(pool.size() == g->vertices.size());

        shuffle(begin(pool), end(pool), random);

        for (int i = 0; i < g->vertices.size(); ++i) {
            rwInfo[i] = pool[i];

            for (int j = 0; j < i; ++j) {
                if (get<0>(rwInfo[i]) == get<0>(rwInfo[j]) &&
                    (get<1>(rwInfo[i]) || get<1>(rwInfo[j]))) {
                    g->AddUndirectedEdge(g->vertices[i], g->vertices[j]);
                }
            }
        }
    }

    static int DsFindRoot(vector<int> & d, int e) {
        if (d[e] != e) {
            return d[e] = DsFindRoot(d, d[e]);
        }
        else {
            return e;
        }
    }

    static void DsMerge(vector<int> & d, int a, int b) {
        d[DsFindRoot(d, a)] = DsFindRoot(d, b);
    }

    void RandomTreeInternal(Graph * g, random_engine & random,
                            const vector<Vertex *> & vertices,
                            void(* funcAddEdge)(Graph *, Vertex *, Vertex *)) {
        vector<int> dsParent;
        int disjointParts = g->vertices.size();
        dsParent.resize(g->vertices.size());

        for (int i = 0; i < dsParent.size(); ++i) {
            dsParent[i] = i;
        }

        vector<tuple<int, int>> edges;
        for (int i = 0; i < dsParent.size(); ++i) {
            for (int j = i + 1; j < dsParent.size(); ++j) {
                edges.push_back(make_tuple(i, j));
            }
        }

        for (int i = 0; i < edges.size() - 1; ++i) {
            uniform_int_distribution<int> d(i + 1, edges.size() - 1);
            swap(edges[i], edges[d(random)]);
        }

        for (int i = 0; i < edges.size(); ++i) {
            int a = DsFindRoot(dsParent, get<0>(edges[i]));
            int b = DsFindRoot(dsParent, get<1>(edges[i]));
            if (a == b)
                continue;
            funcAddEdge(g, vertices[a], vertices[b]);
            DsMerge(dsParent, a, b);
            --disjointParts;
        }

        assert(disjointParts == 1);
    }

    void RandomTree(Graph * g, random_engine & random) {
        RandomTreeInternal(g, random, g->vertices,
                           [](Graph * g, Vertex * a, Vertex * b) {
                               g->AddUndirectedEdge(a, b);
                           });
    }

    void RandomDag(Graph * g, random_engine & random, int leave) {
        
    }
}
