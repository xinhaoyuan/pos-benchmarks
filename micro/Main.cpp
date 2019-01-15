#include "Base.hpp"
#include "Generators.hpp"
#include "Schedulers.hpp"
#include "PorStat.hpp"
#include <cassert>
#include <iostream>
#include <string>
#include <regex>

#define DBG_MAIN 0

using namespace std;
using namespace Generator;

int main(int argc, char ** argv) {
    Graph * g = new Graph();
    PorTree * porTree;
    map<string, string> opts;
    vector<string> evList;

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

    int passes = -1;
    int report = -1;
    int minHit = 10;
    if (getenv("MIN_HIT")) {
        minHit = atoi(getenv("MIN_HIT"));
    }
    string name = "star";
    random_engine random;

    if (opts.find("passes") != opts.end()) {
        passes = stoi(opts["passes"]);
    }

    if (opts.find("progress-report") != opts.end()) {
        report = stoi(opts["progress-report"]);
    }

    if (opts.find("min-hit") != opts.end()) {
        minHit = stoi(opts["min-hit"]);
    }

    long long seed;
    if (opts.find("seed") != opts.end()) {
        seed = stoll(opts["seed"]);
    }
    else {
        random_device rd;
        seed = rd();
    }

    cout << "seed: " << seed << endl;
    random.seed(seed);

    if (opts.find("name") != opts.end()) {
        name = opts["name"];
    }

    if (name == "star") {
        int fanout = 8;
        if (opts.find("star.fanout") != opts.end()) {
            fanout = stoi(opts["star.fanout"]);
        }
        Star(g, 8);
    }
    else if (name == "anti-chain") {
        int acSize = 8;
        if (opts.find("ac.size") != opts.end()) {
            acSize = stoi(opts["ac.size"]);
        }
        AntiChain(g, acSize);
    }
    else if (name == "rainbow") {
        int width = 4;
        int length = 4;
        if (opts.find("rb.width") != opts.end()) {
            width = stoi(opts["rb.width"]);
        }
        if (opts.find("rb.length") != opts.end()) {
            length = stoi(opts["rb.length"]);
        }
        RainbowSkeleton(g, width, length);
    }
    else if (name == "double-tree") {
        int depth = 4;
        if (opts.find("dt.depth") != opts.end()) {
            depth = stoi(opts["dt.depth"]);
        }
        DoubleTreeSkeleton(g, depth);
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

            map<int, tuple<int, bool>> rwInfo; // not used yet
            AddRWDependency(g, random, idleRatio, rwRatio, skew, total, rwInfo);
        }
    }

    int groundTruth;
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

            cerr << porTree->GetRoot()->size << endl;
        }
        e->End();
        cout << "total: " << porTree->GetRoot()->size << endl;
        groundTruth = porTree->GetRoot()->size;
        delete porTree;
    }

    if (report == 0) report = groundTruth;

    for (auto && algoName : evList) {
        porTree = new PorTree(g);
        int passCount = 0;
        random_engine algoRandom(random());

        while ((passes < 0 || passCount < passes) &&
               (porTree->GetRoot()->size < groundTruth || porTree->GetRoot()->minHit < minHit)) {
            map<Vertex *, int> orderMap;
            vector<Vertex *> order;

            if ((passCount + 1) % report == 0) {
                cerr << porTree->GetRoot()->size << '(' << groundTruth << ") "
                     << porTree->GetRoot()->minHit << '(' << minHit << ") "
                     << passCount + 1 << endl;
            }

            if (algoName == "random-walk.basic") {
                RandomWalk::Basic(g, algoRandom, orderMap, order);
            }
            else if (algoName == "pos.basic") {
                Pos::Basic(g, algoRandom, orderMap, order);
            }
            else if (algoName == "pos.dep-based") {
                Pos::DependencyBased(g, algoRandom, orderMap, order);
            }

            DBG(DBG_MAIN, {
                    bool first = true;
                    for (auto v : order) {
                        if (first) first = false;
                        else cout << ',';
                        cout << v->id;
                    }
                    cout << endl;
                });

            porTree->AddPath(order);
            ++passCount;
        }

        cout << algoName << ": " << porTree->GetRoot()->size
             << ' ' << porTree->GetRoot()->minHit
             << ' ' << passCount << endl;
        delete porTree;
    }

    return 0;
}
