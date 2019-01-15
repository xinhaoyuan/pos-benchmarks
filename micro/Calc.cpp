#include "Base.hpp"
#include "Schedulers.hpp"
#include "PorStat.hpp"
#include <cassert>
#include <iostream>
#include <string>
#include <regex>
#include <vector>
#include <set>
#include <map>
#include <fstream>
#include <unistd.h>

#define DBG_CALC 0
#define PCT_DUMMY_START 1

using namespace std;

typedef vector<int> MulFactor;
struct AddFactor {
    vector<MulFactor> factors;
};
map<PorNode *, vector<Vertex *>> trace;
map<PorNode *, set<tuple<Vertex *, Vertex *>>> races;
map<PorNode *, AddFactor> rwBound;
map<PorNode *, AddFactor> bposBound;
map<PorNode *, AddFactor> posBound;
map<PorNode *, AddFactor> pctBound;
map<PorNode *, AddFactor> raposSample;
map<PorNode *, AddFactor> bposSample;
map<PorNode *, AddFactor> posSample;
map<PorNode *, AddFactor> rposSample;
map<PorNode *, int> preemptionNeeded;
map<int, int> preemptionStat;

ostream & operator<<(ostream & o, const AddFactor & f) {
    o << '[';
    bool first = true;
    for (auto && m : f.factors) {
        if (first) first = false;
        else o << ',';
        o << '(';
        {
            bool first = true;
            for (auto && i : m) {
                if (first) first = false;
                else o << ',';
                o << i;
            }
        }
        o << ')';
    }
    o << ']';
}

double Calc(const AddFactor & f) {
    double r = 0;
    for (auto && m : f.factors) {
        double d = 1;
        for (auto && i : m) {
            d = d / i;
        }
        r = r + d;
    }
    return r;
}

ostream & operator<<(ostream & o, const set<Vertex *> & s) {
    o << '{';
    bool first = true;
    for (auto v : s) {
        if (first) first = false;
        else o << ',';
        o << v->id;
    }
    o << '}';
}

int GetRaces(Graph * g, const vector<Vertex *> & o, set<tuple<Vertex *, Vertex *>> & races) {
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

    for (int i = 0; i < o.size(); ++i) {
        assert(frontier.size() > 0);
        assert(frontier.find(o[i]) != end(frontier));

        auto choice = o[i];
        for (auto e : choice->outEdges) {
            if (e->IsDirected()) {
                if (--inDegree[e->to] == 0) {
                    frontier.insert(e->to);
                }
            }
            else if (frontier.find(e->to) != end(frontier)) {
                races.insert(make_tuple(choice, e->to));
            }
        }

        frontier.erase(choice);
    }

    assert(frontier.size() == 0);
}

int GetPreemption(Graph * g, const vector<Vertex *> & o) {
    int ret = 0;
    map<Vertex *, int> inDegree;
    set<Vertex *> frontier;
    set<Vertex *> freshFrontier;

    for (auto v : g->vertices) {
        int d = 0;
        for (auto e : v->inEdges) {
            if (e->IsDirected()) {
                ++d;
            }
        }

        inDegree[v] = d;
        if (d == 0) {
            freshFrontier.insert(v);
        }
    }

    for (int i = 0; i < o.size(); ++i) {
        frontier.insert(begin(freshFrontier), end(freshFrontier));
        assert(frontier.size() > 0);
        assert(frontier.find(o[i]) != end(frontier));

        if (freshFrontier.find(o[i]) == end(freshFrontier) && freshFrontier.size() > 0) {
            ++ret;
        }

        freshFrontier.clear();

        auto choice = o[i];
        for (auto e : choice->outEdges) {
            if (e->IsDirected()) {
                if (--inDegree[e->to] == 0) {
                    freshFrontier.insert(e->to);
                }
            }
        }

        frontier.erase(choice);
    }

    assert(frontier.size() == 0);
    return ret;
}

void AccountRWBound(AddFactor & f, Graph * g, const vector<Vertex *> & o) {
    MulFactor cur;
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

    for (int i = 0; i < o.size(); ++i) {
        assert(frontier.size() > 0);
        assert(frontier.find(o[i]) != end(frontier));
        cur.push_back(frontier.size());

        auto choice = o[i];
        for (auto e : choice->outEdges) {
            if (e->IsDirected()) {
                if (--inDegree[e->to] == 0) {
                    frontier.insert(e->to);
                }
            }
        }

        frontier.erase(choice);
    }

    assert(frontier.size() == 0);

    f.factors.push_back(cur);
}

void AccountBPOSBound(AddFactor & f, Graph * g, const vector<Vertex *> & o) {
    MulFactor cur;
    set<Vertex *> scheduled;
    set<Vertex *> frontier;
    map<Vertex *, set<Vertex *>> happensBefore;
    map<Vertex *, set<Vertex *>> startsBefore;
    map<Vertex *, set<Vertex *>> priDep;
    map<Vertex *, int> inDegree;

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

    for (int i = 0; i < o.size(); ++i) {
        assert(frontier.size() > 0);
        assert(frontier.find(o[i]) != end(frontier));
        auto choice = o[i];

        for (auto e : choice->outEdges) {
            if (!e->IsDirected() &&
                scheduled.find(e->to) != end(scheduled) &&
                startsBefore[choice].find(e->to) == end(startsBefore[choice])) {

                priDep[choice].insert(e->to);
                priDep[choice].insert(priDep[e->to].begin(), priDep[e->to].end());
                for (auto v : startsBefore[e->to]) {
                    if (startsBefore[choice].find(v) == end(startsBefore[choice])) {
                        priDep[choice].insert(v);
                        priDep[choice].insert(priDep[v].begin(), priDep[v].end());
                    }
                }
                happensBefore[choice].insert(e->to);
                happensBefore[choice].insert(happensBefore[e->to].begin(), happensBefore[e->to].end());
            }
        }

        for (auto e : choice->outEdges) {
            if (e->IsDirected()) {
                happensBefore[e->to].insert(choice);
                happensBefore[e->to].insert(happensBefore[choice].begin(), happensBefore[choice].end());

                if (--inDegree[e->to] == 0) {
                    startsBefore[e->to] = happensBefore[e->to];
                    frontier.insert(e->to);
                }
            }
        }

        cur.push_back(priDep[choice].size() + 1);

        frontier.erase(choice);
        scheduled.insert(choice);

        // cout << choice->id << ' ' << happensBefore[choice] << ' ' << startsBefore[choice] << ' ' << priDep[choice] << endl;
    }

    f.factors.push_back(cur);
}

void AccountPOSBound(AddFactor & f, Graph * g, const vector<Vertex *> & o) {
    MulFactor cur;
    set<Vertex *> scheduled;
    set<Vertex *> frontier;
    map<Vertex *, set<Vertex *>> happensBefore;
    map<Vertex *, set<Vertex *>> startsBefore;
    map<Vertex *, int> inDegree;

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

    for (int i = 0; i < o.size(); ++i) {
        assert(frontier.size() > 0);
        assert(frontier.find(o[i]) != end(frontier));
        auto choice = o[i];

        int updCount = 0;

        for (auto e : choice->outEdges) {
            if (!e->IsDirected() &&
                scheduled.find(e->to) != end(scheduled) &&
                startsBefore[choice].find(e->to) == end(startsBefore[choice])) {

                ++updCount;

                happensBefore[choice].insert(e->to);
                happensBefore[choice].insert(happensBefore[e->to].begin(), happensBefore[e->to].end());
            }
        }

        for (auto e : choice->outEdges) {
            if (e->IsDirected()) {
                happensBefore[e->to].insert(choice);
                happensBefore[e->to].insert(happensBefore[choice].begin(), happensBefore[choice].end());

                if (--inDegree[e->to] == 0) {
                    startsBefore[e->to] = happensBefore[e->to];
                    frontier.insert(e->to);
                }
            }
        }

        if (updCount > 0) {
            int pSize = 0;
            for (auto v : happensBefore[choice]) {
                if (startsBefore[choice].find(v) == end(startsBefore[choice])) {
                    ++pSize;
                }
            }

            int rem = pSize % updCount;
            int d = 1;
            for (int i = 0; i < updCount; ++i) {
                if (i < rem) d *= pSize / updCount + 2;
                else d *= pSize / updCount + 1;
            }
            cur.push_back(d);
        }

        frontier.erase(choice);
        scheduled.insert(choice);

        // cout << choice->id << ' ' << happensBefore[choice] << ' ' << startsBefore[choice] << ' ' << priDep[choice] << endl;
    }

    f.factors.push_back(cur);
}

void PCTSample(Graph * g, const map<Vertex *, int> & threadId, const vector<int> & initPri, const vector<int> & dp, vector<Vertex *> & order) {
    order.clear();

    vector<int> pri = initPri;
    vector<bool> started;
#if PCT_DUMMY_START // adding dummy event at the start of threads
    started.resize(pri.size(), false);
#else
    started.resize(pri.size(), true);
#endif

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

    int step = 0;
    while (frontier.size() > 0) {
        Vertex * choice = nullptr;
        int choiceT;
        for (auto v : frontier) {
            int t = threadId.at(v);
            if (choice == nullptr || pri[t] > pri[choiceT]) {
                choice = v;
                choiceT = t;
            }
        }

        int dpIndex = -1;
        for (int i = 0; i < dp.size(); ++i) {
            if (dp[i] == step) {
                dpIndex = i;
                break;
            }
        }

        if (dpIndex >= 0) {
            // cerr << "delay " << choiceT << " to " << (-1 - dpIndex) << endl;
            pri[choiceT] = -(1 + dpIndex);
        }

        if (started[choiceT]) {
            for (auto e : choice->outEdges) {
                if (e->IsDirected()) {
                    if (--inDegree[e->to] == 0) {
                        frontier.insert(e->to);
                    }
                }
            }

            frontier.erase(choice);
            order.push_back(choice);
        }
        else {
            started[choiceT] = true;
        }
        ++step;
    }
}

int main(int argc, char ** argv) {
    Graph * g = new Graph();
    Graph * gr = new Graph(); // with extra read-read dep

    map<string, int> nameToId;
    map<int, string> idToName;

    string line;
    bool rrDep = false;
    while (getline(cin, line)) {
        if (line.size() > 0 && line[0] == '#') {
            if (line.find("# RRDEP") == 0)
                rrDep = true;
            continue;
        }

        stringstream ss(line);
        string src, dst;
        int dir;
        if (!(ss >> src >> dst >> dir)) continue;

        if (nameToId.find(src) == end(nameToId)) {
            g->NewVertex();
            gr->NewVertex();
            int id = nameToId.size();
            nameToId[src] = id;
            idToName[nameToId[src]] = src;
        }

        if (nameToId.find(dst) == end(nameToId)) {
            g->NewVertex();
            gr->NewVertex();
            int id = nameToId.size();
            nameToId[dst] = id;
            idToName[nameToId[dst]] = dst;
        }

        if (!rrDep) g->AddEdge(g->vertices[nameToId[src]], g->vertices[nameToId[dst]], !!dir);
        gr->AddEdge(gr->vertices[nameToId[src]], gr->vertices[nameToId[dst]], !!dir);
    }

    long toCount = 0;
    auto porTree = new PorTree(g);
    auto e = Systematic::CreateDfsExplorer(false);
    e->Begin(g);
    vector<Vertex *> order;
    while (true) {
        if (!e->Explore(order)) break;
        DBG(DBG_CALC, {
                bool first = true;
                for (auto v : order) {
                    if (first) first = false;
                    else cout << ',';
                    cout << v->id;
                }
                cout << endl;
            });

        auto poNode = porTree->AddPath(order);
        AccountRWBound(rwBound[poNode], g, order);
        int pmpt = GetPreemption(g, order);
        {
            GetRaces(g, order, races[poNode]);
        }
        if (preemptionNeeded.find(poNode) == end(preemptionNeeded) ||
            preemptionNeeded[poNode] > pmpt) {
            preemptionNeeded[poNode] = pmpt;
        }
        if (poNode->minHit == 1) {
            trace[poNode] = order;
            AccountBPOSBound(bposBound[poNode], g, order);
            AccountPOSBound(posBound[poNode], g, order);
        }
        ++toCount;

        if (toCount % 1000000 == 0)
            cerr << getpid() << ':' << toCount << endl;
    }
    e->End();

    cout << "Total Order Count: " << toCount << endl;
    int max_preemption = -1;
    for (auto && kv : preemptionNeeded) {
        if (max_preemption < 0 || max_preemption < get<1>(kv)) {
            max_preemption = get<1>(kv);
        }
    }

    int maxRaces = -1;
    for (auto && kv : races) {
        if (maxRaces < 0 || maxRaces < get<1>(kv).size()) {
            maxRaces = get<1>(kv).size();
        }
    }
    cout << "Max Preemptions: " << max_preemption << endl;
    cout << "Max Races: " << maxRaces << endl;
    cout << "Total PO traces: " << porTree->GetRoot()->size << endl;

    bool hasPCT = false;
    // accounting for PCT
    if (getenv("CALC_PCT_PARAM")) {
        stringstream ss(getenv("CALC_PCT_PARAM"));
        int pct_n;
        int pct_d;
        int sample_count;
        long seed;
        ss >> pct_n >> pct_d >> sample_count;
        if (sample_count > 0) {
            ss >> seed;
        }

        map<char, int> tcToId;
        for (auto v : g->vertices) {
            char tc = idToName[v->id][0];
            if (tcToId.find(tc) == end(tcToId)) {
                int id = tcToId.size();
                tcToId[tc] = id;
            }
        }

        map<Vertex *, int> threadId;
        for (auto v : g->vertices) {
            threadId[v] = tcToId[idToName[v->id][0]];
        }

        if (pct_n <= 0) pct_n = g->vertices.size();
        if (pct_d < 0) {
            pct_d = max_preemption - 1 - pct_d;
        }

        if (sample_count <= 0) {
            vector<int> threadInitPri;
            for (int i = 0; i < tcToId.size(); ++i) {
                threadInitPri.push_back(i);
            }

            do {
                vector<int> dp;
                dp.resize(pct_d, 0);
                while (true) {
                    // {
                    //     cout << "th p ";
                    //     bool first = true;
                    //     for (auto p : threadInitPri) {
                    //         if (first) first = false;
                    //         else cout << ',';
                    //         cout << p;
                    //     }
                    //     cout << endl;
                    // }
                    // {
                    //     cout << "delay point ";
                    //     bool first = true;
                    //     for (auto p : dp) {
                    //         if (first) first = false;
                    //         else cout << ',';
                    //         cout << p;
                    //     }
                    //     cout << endl;
                    // }

                    PCTSample(g, threadId, threadInitPri, dp, order);

                    // {
                    //     cout << "trace ";
                    //     bool first = true;
                    //     for (auto v : order) {
                    //         if (first) first = false;
                    //         else cout << ',';
                    //         cout << idToName[v->id];
                    //     }
                    //     cout << endl;
                    // }
                    // simulation finished, do accounting and prepare next run
                    auto poNode = porTree->AddPath(order);
                    MulFactor cur;
                    for (int i = 0; i < tcToId.size(); ++i) {
                        cur.push_back(i + 1);
                    }
                    for (int i = 0; i < pct_d; ++i) {
                        cur.push_back(pct_n);
                    }
                    pctBound[poNode].factors.push_back(cur);

                    bool nextRound = false;
                    for (int i = 0; i < dp.size(); ++i) {
#if PCT_DUMMY_START
                        if (dp[i] < pct_n - 1 + threadInitPri.size())
#else
                        if (dp[i] < pct_n - 1)
#endif
                        {
                            for (int j = 0; j < i; ++j) {
                                dp[j] = 0;
                            }
                            ++dp[i];
                            nextRound = true;
                            break;
                        }
                    }

                    if (!nextRound) break;
                }
            } while (next_permutation(threadInitPri.begin(), threadInitPri.end()));
        }
        else {
            random_engine meta_rng(seed);
            for (int itr = 0; itr < sample_count; ++itr) {
                random_engine rng(meta_rng());

                vector<int> threadInitPri;
                for (int i = 0; i < tcToId.size(); ++i) {
                    threadInitPri.push_back(i);
                }
                shuffle(begin(threadInitPri), end(threadInitPri), rng);

#if PCT_DUMMY_START
                uniform_int_distribution<int> dist(0, pct_n - 1 + threadInitPri.size());
#else
                uniform_int_distribution<int> dist(0, pct_n - 1);
#endif

                vector<int> dp;
                for (int i = 0; i < pct_d; ++i) {
                    dp.push_back(dist(rng));
                }

                PCTSample(g, threadId, threadInitPri, dp, order);

                auto poNode = porTree->AddPath(order);
                MulFactor cur;
                cur.push_back(sample_count);
                pctBound[poNode].factors.push_back(cur);
            }
        }

        hasPCT = true;
    }

    bool hasRAPOSSample = false;
    if (getenv("CALC_RAPOS_SAMPLE")) {
        stringstream ss(getenv("CALC_RAPOS_SAMPLE"));
        long times, seed;
        ss >> times >> seed;
        random_engine re(seed);
        for (int i = 0; i < times; ++i) {
            map<Vertex *, int> orderMap;
            random_engine algoRe(re());
            Misc::Rapos(g, algoRe, orderMap, order);

            auto poNode = porTree->AddPath(order);
            assert(poNode->minHit > 1);
            MulFactor f;
            f.push_back(times);
            raposSample[poNode].factors.push_back(f);
        }
        hasRAPOSSample = true;
    }

    bool hasBPOSSample = false;
    if (getenv("CALC_BPOS_SAMPLE")) {
        stringstream ss(getenv("CALC_BPOS_SAMPLE"));
        long times, seed;
        ss >> times >> seed;
        random_engine re(seed);
        for (int i = 0; i < times; ++i) {
            map<Vertex *, int> orderMap;
            random_engine algoRe(re());
            Pos::Basic(g, algoRe, orderMap, order);

            auto poNode = porTree->AddPath(order);
            MulFactor f;
            f.push_back(times);
            bposSample[poNode].factors.push_back(f);
        }
        hasBPOSSample = true;
    }

    bool hasPOSSample = false;
    if (getenv("CALC_POS_SAMPLE")) {
        stringstream ss(getenv("CALC_POS_SAMPLE"));
        long times, seed;
        ss >> times >> seed;
        random_engine re(seed);
        for (int i = 0; i < times; ++i) {
            map<Vertex *, int> orderMap;
            random_engine algoRe(re());
            Pos::DependencyBased(g, algoRe, orderMap, order);

            auto poNode = porTree->AddPath(order);
            MulFactor f;
            f.push_back(times);
            posSample[poNode].factors.push_back(f);
        }
        hasPOSSample = true;
    }

    bool hasRPOSSample = false;
    if (getenv("CALC_RPOS_SAMPLE")) {
        stringstream ss(getenv("CALC_RPOS_SAMPLE"));
        long times, seed;
        ss >> times >> seed;
        random_engine re(seed);
        for (int i = 0; i < times; ++i) {
            map<Vertex *, int> orderMap;
            random_engine algoRe(re());

            Pos::DependencyBased(gr, algoRe, orderMap, order);
            for (int i = 0; i < order.size(); ++i) {
                assert(g->vertices.at(order[i]->id)->id == order[i]->id);
                order[i] = g->vertices.at(order[i]->id);
            }

            auto poNode = porTree->AddPath(order);
            MulFactor f;
            f.push_back(times);
            rposSample[poNode].factors.push_back(f);
        }
        hasRPOSSample = true;
    }

    map<string, double> total;
    map<string, double> min;
    map<string, vector<double>> distribution;
    map<string, int> coverage;

    cout << endl;

    cout << "po trace,preemption,races" << endl;
    for (auto && kv : preemptionNeeded) {
        {
            cout << '"';
            bool first = true;
            for (auto v : trace[get<0>(kv)]) {
                if (first) first = false;
                else cout << "->";
                cout << idToName[v->id];
            }
            cout << '"';
        }
        cout << ',' << get<1>(kv);
        ++preemptionStat[get<1>(kv)];

        cout << ',' << races[get<0>(kv)].size();

        cout << endl;
    }
    cout << endl;

    cout << "preemption,count" << endl;
    for (auto && kv : preemptionStat) {
        cout << get<0>(kv) << ',' << get<1>(kv) << endl;
    }
    cout << endl;

    vector<string> colOrder;
    cout << "po trace,rw";
    colOrder.push_back("RW");
    if (hasPCT) {
        cout << ",pct";
        colOrder.push_back("PCT");
    }
    if (hasRAPOSSample) {
        cout << ",rapos sampled";
        colOrder.push_back("RAPOS-Sample");
    }
    cout << ",bpos bound";
    colOrder.push_back("BPOS");
    if (hasBPOSSample) {
        cout << ",bpos sampled";
        colOrder.push_back("BPOS-Sample");
    }
    cout << ",pos bound";
    colOrder.push_back("POS");
    if (hasPOSSample) {
        cout << ",pos sampled";
        colOrder.push_back("POS-Sample");
    }
    if (hasRPOSSample) {
        cout << ",rpos sampled";
        colOrder.push_back("RPOS-Sample");
    }

    cout << endl;

    for (auto && kv : rwBound) {
        {
            cout << '"';
            bool first = true;
            for (auto v : trace[get<0>(kv)]) {
                if (first) first = false;
                else cout << "->";
                cout << idToName[v->id];
            }
            cout << '"';
        }

        map<string, AddFactor> row;
        row["RW"] = get<1>(kv);
        row["BPOS"] = bposBound[get<0>(kv)];
        row["POS"] = posBound[get<0>(kv)];
        if (hasPCT) row["PCT"] = pctBound[get<0>(kv)];
        if (hasRAPOSSample) row["RAPOS-Sample"] = raposSample[get<0>(kv)];
        if (hasBPOSSample) row["BPOS-Sample"] = bposSample[get<0>(kv)];
        if (hasPOSSample) row["POS-Sample"] = posSample[get<0>(kv)];
        if (hasRPOSSample) row["RPOS-Sample"] = rposSample[get<0>(kv)];

        for (auto && name : colOrder) {
            double p = Calc(row[name]);
            distribution[name].push_back(p);
            cout << ',' << p;
            total[name] += p;
            if (row[name].factors.size() > 0) {
                ++coverage[name];
                if (min.find(name) == end(min) ||
                    min[name] > p) {
                    min[name] = p;
                }
            }
        }
        cout << endl;
    }

    cout << "Total";
    for (auto && name : colOrder) {
        cout << ',' << total[name];
    }
    cout << endl;

    cout << "Coverage";
    for (auto && name : colOrder) {
        cout << ',' << coverage[name];
    }
    cout << endl;

    cout << "Min";
    for (auto && name : colOrder) {
        if (min.find(name) == end(min))
            cout << ",0";
        else cout << ',' << min[name];
    }
    cout << endl;

    cout << "Variance";
    for (auto && name : colOrder) {
        string fname = name;
        fname.append(".csv");
        ofstream out(fname.c_str());

        double variance = 0;

        for (double x : distribution[name]) {
            double diff = (x - (double)total[name] / distribution[name].size());
            variance += diff * diff;
            out << x << ',' << endl;
        }
        out.close();

        cout << ',' << variance;
    }
    cout << endl;

    delete porTree;
}
