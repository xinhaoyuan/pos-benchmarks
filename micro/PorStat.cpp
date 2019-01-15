#include "PorStat.hpp"
#include <algorithm>
#include <set>
#include <cassert>
#include <iostream>

#define DBG_POR_STAT 0

using namespace std;

PorTree::PorTree(Graph * g)
    : _graph(g) {
}

static void FreeSubtree(PorNode * n) {
    for (auto c : n->children) {
        FreeSubtree(c);
        delete c;
    }
}

PorTree::~PorTree() {
    FreeSubtree(&_root);
}

PorNode * PorTree::AddPath(const vector<Vertex *> & path) {
    vector<Vertex *> porPath(path);
    PorNode * cur = &_root;
    size_t pos = 0;
    map<Vertex *, set<Vertex *>> dependencies;
    map<Vertex *, size_t> sleepSet;
    vector<vector<Vertex *>> sleepSetStack;
    vector<PorNode *> nodeStack = { cur };

    {
        map<Vertex *, int> index;
        for (int i = 0; i < path.size(); ++i) {
            index[path[i]] = i;
        }

        for (auto v : _graph->vertices) {
            assert(index.find(v) != index.end());
            for (auto e : v->outEdges) {
                if (e->IsDirected()) {
                    assert(index.find(e->to) != index.end());
                    assert(index[v] < index[e->to]);
                }
            }
        }
    }

    while (pos < porPath.size()) {
        Vertex * v = porPath[pos];

        DBG(DBG_POR_STAT, {
                bool first = true;
                for (auto && kv : sleepSet) {
                    if (first) first = false;
                    else cout << ',';
                    cout << kv.first->id;
                }
                cout << endl;
                cout << pos << ':' << v->id << endl;
            });

        {
            auto it = sleepSet.find(v);
            if (it != sleepSet.end()) {
                size_t btPos = it->second;

                DBG(DBG_POR_STAT, { cout << "swap " << btPos << " and " << pos << endl; });

                for (size_t i = pos; i > btPos; --i) {
                    swap(porPath[i], porPath[i - 1]);
                }

                while (btPos < sleepSetStack.size()) {
                    auto && head = sleepSetStack.back();
                    DBG(DBG_POR_STAT, {
                            cout << "pop sleep set: ";
                            bool first = true;
                            for (auto v : head) {
                                if (first) first = false;
                                else cout << ',';
                                cout << v->id;
                            }
                            cout << endl;
                        });

                    for (auto v : head) {
                        sleepSet.erase(v);
                    }
                    sleepSetStack.pop_back();
                }

                pos = btPos;
                cur = nodeStack[pos];
                nodeStack.resize(pos + 1);
                continue;
            }
        }

        if (cur != nullptr) {
            auto it = cur->index.find(v);
            size_t idx;

            if (it == cur->index.end()) {
                idx = cur->children.size();
            }
            else {
                idx = it->second;
            }

            sleepSetStack.push_back({});
            auto && level = sleepSetStack.back();
            for (size_t i = 0; i < idx; ++i) {
                level.push_back(cur->vertices[i]);
                sleepSet[cur->vertices[i]] = pos;
            }

            if (it == cur->index.end()) {
                cur = nullptr;
            }
            else {
                cur = cur->children[it->second];
                nodeStack.push_back(cur);
            }
        }

        vector<Vertex *> wakeup;
        for (auto && kv : sleepSet) {
            auto depIt = dependencies.find(kv.first);
            if (depIt == dependencies.end()) {
                // find dependency set and cache it
                tie(depIt, std::ignore) = dependencies.emplace(kv.first, set<Vertex *>{});
                for (auto e : kv.first->outEdges) {
                    if (e->IsDirected()) continue;
                    depIt->second.insert(e->to);
                }
            }

            if (depIt->second.find(v) != depIt->second.end()) {
                wakeup.push_back(kv.first);
            }
        }

        for (auto v : wakeup) {
            sleepSet.erase(v);
        }

        ++pos;
    }

    DBG(DBG_POR_STAT, {
            cout << "path after partial order reduction: ";
            bool first = true;
            for (auto v : porPath) {
                if (first) first = false;
                else cout << ',';
                cout << v->id;
            }
            cout << endl;
        });

    pos = 0;
    sleepSet.clear();
    cur = &_root;
    nodeStack.clear();
    nodeStack.push_back(cur);
    bool newPath = false;

    while (pos < porPath.size()) {
        Vertex * v = porPath[pos];

        assert(sleepSet.find(v) == sleepSet.end());
        auto it = cur->index.find(v);
        size_t idx;

        if (it == cur->index.end()) {
            idx = cur->children.size();
            tie(it, std::ignore) = cur->index.emplace(v, idx);
            cur->vertices.push_back(v);
            cur->children.push_back(new PorNode());

            newPath = true;
        }
        else {
            idx = it->second;
        }

        for (size_t i = 0; i < idx; ++i) {
            sleepSet[cur->vertices[i]] = pos;
        }

        vector<Vertex *> wakeup;
        for (auto && kv : sleepSet) {
            auto depIt = dependencies.find(kv.first);
            if (depIt == dependencies.end()) {
                // find dependency set and cache it
                tie(depIt, std::ignore) = dependencies.emplace(kv.first, set<Vertex *>{});
                for (auto e : kv.first->outEdges) {
                    if (e->IsDirected()) continue;
                    depIt->second.insert(e->to);
                }
            }

            if (depIt->second.find(v) != depIt->second.end()) {
                wakeup.push_back(kv.first);
            }
        }

        for (auto v : wakeup) {
            sleepSet.erase(v);
        }

        cur = cur->children[it->second];
        ++pos;

        nodeStack.push_back(cur);
    }

    auto ret = nodeStack.back();

    while (nodeStack.size() > 0) {
        if (newPath)
            ++nodeStack.back()->size;
        ++nodeStack.back()->minHit;
        for (auto c : nodeStack.back()->children) {
            if (c->minHit < nodeStack.back()->minHit) {
                nodeStack.back()->minHit = c->minHit;
            }
        }
        nodeStack.pop_back();
    }

    return ret;
}
