#include "Schedulers.hpp"
#include <iostream>
#include <random>
#include <map>
#include <set>
#include <cassert>

#define DBG_SCH 0
#define SANITY_CHECK 0

using namespace std;

namespace Systematic {
    class DfsExplorer : public IExplorer {
        Graph * _graph;

        struct ExplNode {
            map<Vertex *, int> index;
            vector<Vertex *>   vertices;
        };

        bool _fSleepSet;
        vector<ExplNode> _stack;

    public:
        DfsExplorer()
            : _fSleepSet(true)
            { }

        void Begin(Graph * g) override {
            _graph = g;
            _stack.push_back(ExplNode());
        }

        void SetUseSleepSet(bool use) {
            _fSleepSet = use;
        }

        bool Explore(vector<Vertex *> & outOrder) override {
            if (_stack.size() == 0) {
                // Entire tree exhausted
                return false;
            }

            outOrder.clear();

            random_device rd;
            uniform_real_distribution<double> dist(0.0, 1.0);
            mt19937_64 random(rd());

            map<Vertex *, int> inDegree;
            set<Vertex *> frontier;

            bool rejected;
            do {
                int level = 0;
                rejected = false;
                set<Vertex *> sleepSet;
                outOrder.clear();

                frontier.clear();
                inDegree.clear();
                for (auto v : _graph->vertices) {
                    int d = 0;
                    for (auto e : v->inEdges) {
                        if (e->IsDirected()) {
                            ++d;
                        }
                    }

                    inDegree[v] = d;
                    if (d == 0) {
                        frontier.insert(v);
                    }
                }

                while (frontier.size() > 0) {
                    DBG(DBG_SCH, {
                            cout << "frontier: ";
                            bool first = true;
                            for (auto v : frontier) {
                                if (first) first = false;
                                else cout << ' ';
                                cout << v->id;
                            }

                            cout << endl;
                            if (_fSleepSet) {
                                cout << "sleep set: ";
                                first = true;
                                for (auto v : sleepSet) {
                                    if (first) first = false;
                                    else cout << ' ';
                                    cout << v->id;
                                }
                            }
                            cout << endl;
                        });

                    Vertex * choice = nullptr;

                    if (level == _stack.size() - 1) {
                        // subtree is exhausted, pick some one that is not in index or sleep set
                        for (auto v : frontier) {
                            if (_stack[level].index.find(v) != _stack[level].index.end())
                                continue;
                            if (_fSleepSet && sleepSet.find(v) != sleepSet.end())
                                continue;
                            choice = v;
                            break;
                        }

                        if (choice == nullptr) {
                            // the current level is exhausted
                            _stack.pop_back();
                            rejected = true;
                            break;
                        }
                        else {
                            if (_fSleepSet) {
                                for (auto v : _stack[level].vertices) {
                                    sleepSet.insert(v);
                                }
                            }

                            _stack[level].index[choice] = _stack[level].vertices.size();
                            _stack[level].vertices.push_back(choice);
                            _stack.push_back(ExplNode());
                        }
                    }
                    else {
                        if (_fSleepSet) {
                            for (int i = 0; i < _stack[level].vertices.size() - 1; ++i) {
                                sleepSet.insert(_stack[level].vertices[i]);
                            }
                        }
                        choice = _stack[level].vertices.back();
                    }

                    DBG(DBG_SCH, cout << choice->id << endl);

                    for (auto e : choice->outEdges) {
                        if (e->IsDirected()) {
                            if (--inDegree[e->to] == 0) {
                                frontier.insert(e->to);
                            }
                        }
                        else {
                            if (_fSleepSet) {
                                sleepSet.erase(e->to);
                            }
                        }
                    }

                    ++level;
                    frontier.erase(choice);
                    outOrder.push_back(choice);
                }
            }
            while (rejected && _stack.size() > 0);

            if (rejected) {
                return false;
            }
            else {
                DBG(DBG_SCH, {
                        bool first = true;
                        for (auto v : outOrder) {
                            if (first) first = false;
                            else cout << ' ';
                            cout << v->id;
                        }
                        cout << endl;
                    });
                // the last node is a leaf, thus exhausted
                _stack.pop_back();
                return true;
            }
        }

        void End() override {
            _graph = nullptr;
            _stack.clear();
        }

        ~DfsExplorer() override {
        }
    };

    IExplorer * CreateDfsExplorer(bool sleepSet) {
        auto ret = new DfsExplorer();
        ret->SetUseSleepSet(sleepSet);
        return ret;
    }
}

void RandomWalk::Basic(Graph * g, random_engine & random, map<Vertex *, int> & outOrderMap, vector<Vertex *> & outOrder) {
    outOrderMap.clear();
    outOrder.clear();

    uniform_real_distribution<double> dist(0.0, 1.0);

    map<Vertex *, int> inDegree;
    set<Vertex *> frontier;

    for (auto v : g->vertices) {
        int d = 0;
        for (auto e : v->inEdges) {
            if (e->IsDirected()) {
                ++d;
            }
        }

        inDegree[v] = d;
        if (d == 0) {
            frontier.insert(v);
        }
    }

    while (frontier.size() > 0) {
        Vertex * choice = nullptr;
        double bestP;

        for (auto v : frontier) {
            double p = dist(random);

            if (choice == nullptr || bestP < p) {
                choice = v;
                bestP = p;
            }
        }

        for (auto e : choice->outEdges) {
            if (e->IsDirected()) {
                if (--inDegree[e->to] == 0) {
                    frontier.insert(e->to);
                }
            }
        }

        frontier.erase(choice);
        outOrderMap[choice] = outOrder.size();
        outOrder.push_back(choice);
    }
}

void Pos::Basic(Graph * g, random_engine & random, map<Vertex *, int> & outOrderMap, vector<Vertex *> & outOrder) {
    outOrderMap.clear();
    outOrder.clear();

    map<Vertex *, double> priority;
    uniform_real_distribution<double> dist(0.0, 1.0);

    map<Vertex *, int> inDegree;
    set<Vertex *> frontier;

    for (auto v : g->vertices) {
        int d = 0;
        for (auto e : v->inEdges) {
            if (e->IsDirected()) {
                ++d;
            }
        }

        inDegree[v] = d;
        if (d == 0) {
            frontier.insert(v);
        }
    }

    while (frontier.size() > 0) {
        Vertex * choice = nullptr;
        double bestP;

        for (auto v : frontier) {
            double p;
            if (priority.find(v) == priority.end()) {
                p = priority[v] = dist(random);
            }
            else {
                p = priority[v];
            }

            if (choice == nullptr || bestP < p) {
                choice = v;
                bestP = p;
            }
        }

        for (auto e : choice->outEdges) {
            if (e->IsDirected()) {
                if (--inDegree[e->to] == 0) {
                    frontier.insert(e->to);
                }
            }
        }

        frontier.erase(choice);
        outOrderMap[choice] = outOrder.size();
        outOrder.push_back(choice);
    }
}

void Pos::DependencyBased(Graph * g, random_engine & random, std::map<Vertex *, int> & outOrderMap, std::vector<Vertex *> & outOrder) {
    outOrderMap.clear();
    outOrder.clear();

    map<Vertex *, double> priority;
    uniform_real_distribution<double> dist(0.0, 1.0);

    map<Vertex *, int> inDegree;
    set<Vertex *> frontier;

    for (auto v : g->vertices) {
        int d = 0;
        for (auto e : v->inEdges) {
            if (e->IsDirected()) {
                ++d;
            }
        }

        inDegree[v] = d;
        if (d == 0) {
            frontier.insert(v);
        }
    }

    while (frontier.size() > 0) {
        Vertex * choice = nullptr;
        double bestP;

        for (auto v : frontier) {
            double p;
            if (priority.find(v) == priority.end()) {
                p = priority[v] = dist(random);
            }
            else {
                p = priority[v];
            }

            if (choice == nullptr || bestP < p) {
                choice = v;
                bestP = p;
            }
        }

        for (auto e : choice->outEdges) {
            if (e->IsDirected()) {
                if (--inDegree[e->to] == 0) {
                    frontier.insert(e->to);
                }
            }
            else {
                priority.erase(e->to);
            }
        }

        frontier.erase(choice);
        outOrderMap[choice] = outOrder.size();
        outOrder.push_back(choice);
    }
}

void Misc::Rapos(Graph * g, random_engine & random, map<Vertex *, int> & outOrderMap, vector<Vertex *> & outOrder) {
    outOrderMap.clear();
    outOrder.clear();

    uniform_real_distribution<double> dist(0.0, 1.0);
    map<Vertex *, int> inDegree;
    set<Vertex *> frontier;

    for (auto v : g->vertices) {
        int d = 0;
        for (auto e : v->inEdges) {
            if (e->IsDirected()) {
                ++d;
            }
        }

        inDegree[v] = d;
        if (d == 0) {
            frontier.insert(v);
        }
    }

    map<Vertex *, set<Vertex *>> dep;

    for (auto e : g->edges) {
        if (!e->IsDirected()) {
            dep[e->from].insert(e->to);
        }
    }

    vector<Vertex *> schedulable(frontier.begin(), frontier.end());

    while (frontier.size() > 0) {
        assert(schedulable.size() > 0);

        vector<Vertex *> scheduled;
        {
            uniform_int_distribution<int> dist(0, schedulable.size() - 1);
            scheduled.push_back(schedulable[dist(random)]);
        }

        for (int i = 0; i < schedulable.size(); ++i) {
            assert(frontier.find(schedulable[i]) != end(frontier));

            bool isIndependent = true;
            for (auto v : scheduled) {
                if (v == schedulable[i] ||
                    (dep.find(v) != end(dep) && dep[v].find(schedulable[i]) != end(dep[v]))) {
                    isIndependent = false;
                    break;
                }
            }

            if (isIndependent && dist(random) <= 0.5) {
                scheduled.push_back(schedulable[i]);
            }
        }

        set<Vertex *> inactive = frontier;

        for (int i = 0; i < scheduled.size(); ++i) {
            Vertex * choice = scheduled[i];
            assert(frontier.find(choice) != end(frontier));

            for (auto e : choice->outEdges) {
                if (e->IsDirected()) {
                    if (--inDegree[e->to] == 0) {
                        frontier.insert(e->to);
                    }
                }
                else {
                    if (inactive.find(e->to) != end(inactive)) {
                        inactive.erase(e->to);
                    }
                }
            }

            inactive.erase(choice);
            frontier.erase(choice);
            outOrderMap[choice] = outOrder.size();
            outOrder.push_back(choice);
        }

        schedulable.clear();
        if (frontier.size() > 0) {
            uniform_int_distribution<int> dist(0, frontier.size() - 1);
            Vertex * backup = nullptr;
            int backupIndex = dist(random);
            int index = 0;
            for (auto v : frontier) {
                if (index == backupIndex) {
                    backup = v;
                }
                ++index;

                if (inactive.find(v) == end(inactive)) {
                    schedulable.push_back(v);
                }
            }

            if (schedulable.size() == 0) {
                assert(backup != nullptr);
                schedulable.push_back(backup);
            }
        }
    }
}
