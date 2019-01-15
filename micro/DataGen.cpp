#include "Base.hpp"
#include "Generators.hpp"
#include "Schedulers.hpp"
#include "PorStat.hpp"
#include <cassert>
#include <iostream>
#include <string>
#include <set>
#include <map>
#include <vector>
#include <regex>

#define DBG_MAIN 0

using namespace std;
using namespace Generator;

int main(int argc, char ** argv) {
    Graph * g = new Graph();
    PorTree * porTree;
    map<string, string> opts;
    vector<string> evList;
    map<int, tuple<int, bool>> rwInfo;

    {
        regex reKv("([-_a-zA-Z.0-9]+)=(.*)");
        regex reSwitch("-([-_a-zA-Z.0-9]+)");
        for (int i = 1; i < argc; ++i) {
            smatch m;
            string arg(argv[i]);
            if (regex_match(arg, m, reKv)) {
                if (arg[0] == '-') {
                    // noop
                }
                else {
                    opts[m[1]] = m[2];
                }
            }
            else if (regex_match(arg, m, reSwitch)) {
                opts[m[1]] = "1";
            }
            else {
                evList.push_back(arg);
            }
        }
    }

    string name = "star";
    random_engine random;

    long long seed;
    if (opts.find("seed") != opts.end()) {
        seed = stoll(opts["seed"]);
    }
    else {
        random_device rd;
        seed = rd();
    }

    cout << "# opts:" << endl;
    for (auto && kv : opts) {
        cout << "#   " << get<0>(kv) << ':' << get<1>(kv) << endl;
    }

    cout << "# seed: " << seed << endl;
    random.seed(seed);

    if (opts.find("name") != opts.end()) {
        name = opts["name"];
    }

    map<Vertex *, int> threadId;

    if (name == "rainbow") {
        int width = 4;
        int length = 4;
        if (opts.find("rb.width") != opts.end()) {
            width = stoi(opts["rb.width"]);
        }
        if (opts.find("rb.length") != opts.end()) {
            length = stoi(opts["rb.length"]);
        }
        RainbowSkeleton(g, width, length);

        for (auto v : g->vertices) {
            threadId[v] = v->id / length;
        }
    }
    else if (name == "double-tree") {
        int depth = 4;
        if (opts.find("dt.depth") != opts.end()) {
            depth = stoi(opts["dt.depth"]);
        }
        DoubleTreeSkeleton(g, depth);

        {
            // tricky to define thread ids
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

            assert(frontier.size() == 1);
            threadId[*frontier.begin()] = 0;

            int level = 0;
            while (frontier.size() > 0) {
                ++level;
                vector<Vertex *> fr(frontier.begin(), frontier.end());
                frontier.clear();

                for (auto v : fr) {
                    int childCount = 0;
                    for (auto e : v->outEdges) {
                        if (e->IsDirected()) {
                            int id = threadId[v] + childCount * (1 << (depth - level));
                            if (threadId.find(e->to) == threadId.end() || threadId[e->to] > id) {
                                threadId[e->to] = id;
                            }
                            ++childCount;

                            if (--inDegree[e->to] == 0) {
                                frontier.insert(e->to);
                            }
                        }
                    }
                }
            }
        }
    }

    if (name == "anti-chain" || name == "rainbow" || name == "double-tree") {
        string depName = "uniform";

        if (opts.find("dep-name") != opts.end()) {
            depName = opts["dep-name"];
        }

        if (depName == "uniform") {
            double prob = 0.5;
            if (opts.find("uniform.prob") != opts.end()) {
                prob = stod(opts["uniform.prob"]);
            }
            AddUniformPairDependency(g, random, prob);
        }
        else if (depName == "rw") {
            double idleRatio = 0.2;
            double rwRatio = 0.5;
            double skew = 0.3;
            int total = 20;

            if (opts.find("rw.ratio") != opts.end()) {
                rwRatio = stod(opts["rw.ratio"]);
            }

            if (opts.find("rw.skew") != opts.end()) {
                skew = stod(opts["rw.skew"]);
            }

            if (opts.find("rw.total") != opts.end()) {
                total = stoi(opts["rw.total"]);
            }

            if (opts.find("rw.idle") != opts.end()) {
                idleRatio = stod(opts["rw.idle"]);
            }

            AddRWDependency(g, random, idleRatio, rwRatio, skew, total, rwInfo);
        }
        else if (depName == "rwd") {
            vector<int> rwDist;
            string distStr = opts["rwd.dist"];
            int pos, lastPos = 0;
            while (true) {
                pos = distStr.find(',', lastPos);
                if (pos == string::npos) {
                    rwDist.push_back(stoi(distStr.substr(lastPos)));
                    break;
                }
                else {
                    rwDist.push_back(stoi(distStr.substr(lastPos, pos - lastPos)));
                    lastPos = pos + 1;
                }
            }

            // // For simple debug
            // for (auto i : rwDist) {
            //     cerr << i << ' ';
            // }
            // cerr << endl;

            AddRWDistDependency(g, random, rwDist, rwInfo);
        }
    }

    {
        porTree = new PorTree(g);
        auto e = Systematic::CreateDfsExplorer();
        e->Begin(g);
        vector<Vertex *> order;
        while (true) {
            if (!e->Explore(order)) break;

            DBG(DBG_MAIN, {
                    bool first = true;
                    for (auto v : order) {
                        if (first) first = false;
                        else cout << ',';
                        cout << v->id;
                    }
                    cout << endl;
                });
            int oldSize = porTree->GetRoot()->size;
            porTree->AddPath(order);
            assert(oldSize < porTree->GetRoot()->size);
            assert(porTree->GetRoot()->minHit == 1);
        }
        e->End();
        cout << "# por count: " << porTree->GetRoot()->size << endl;
        delete porTree;
    }

    for (auto v : g->vertices) {
        assert(threadId.find(v) != threadId.end());
    }

    for (auto v : g->vertices) {
        for (auto e : v->outEdges) {
            if (e->IsDirected()) {
                cout << threadId[v] << '_' << v->id << ' ' << threadId[e->to] << '_' << e->to->id << " 1" << endl;
            }
            else if (v->id < e->to->id) {
                cout << threadId[v] << '_' << v->id << ' ' << threadId[e->to] << '_' << e->to->id << " 0" << endl;
            }
        }
    }

    {
        bool first = true;
        for (auto v1 : g->vertices) {
            for (auto v2 : g->vertices) {
                if (v1->id < v2->id &&
                    rwInfo.find(v1->id) != end(rwInfo) &&
                    rwInfo.find(v2->id) != end(rwInfo) &&
                    get<0>(rwInfo[v1->id]) == get<0>(rwInfo[v2->id]) &&
                    !get<1>(rwInfo[v1->id]) && !get<1>(rwInfo[v2->id])) {
                    if (first) {
                        first = false;
                        cout << "# RRDEP" << endl;
                    }
                    cout << threadId[v1] << '_' << v1->id << ' ' << threadId[v2] << '_' << v2->id << " 0" << endl;
                }
            }
        }
    }

    return 0;
}
