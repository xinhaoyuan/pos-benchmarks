#include "Base.hpp"
#include "Generators.hpp"
#include <iostream>
#include <algorithm>

using namespace std;
using namespace Generator;

int main() {
    Graph * g = new Graph();
    random_device rd;
    random_engine random(rd());

    AntiChain(g, 100);
    RandomTree(g, random);

    map<Vertex *, int> count;
    Vertex * now = g->vertices[0];
    Vertex * prev = nullptr;
    for (int i = 0; i < 1000000; ++i) {
        vector<tuple<Vertex *, Vertex *>> path;
        while (true) {
            path.push_back(make_tuple(prev, now));

            int ub = now->outEdges.size() - (prev ? 2 : 1);
            if (ub == -1) break;

            uniform_int_distribution<int> d(0, ub);
            int c = d(random);
            Vertex * next = nullptr;
            for (auto e : now->outEdges) {
                if (e->to != prev) {
                    next = e->to;
                    if (--c == 0) break;
                }
            }

            prev = now;
            now = next;
        }

        path.push_back(make_tuple(now, nullptr));
        ++count[now];

        {
            uniform_int_distribution<int> d(1, path.size() - 1);
            int index = max<int>(1, path.size() - 2);
            now = get<0>(path[index]);
            prev = get<1>(path[index]);
        }
    }

    bool first = true;
    for (auto && kv : count) {
        if (first) first = false;
        else cout << ' ';
        cout << kv.first->id << "," << kv.second;
    }
    cout << endl;

    delete g;
    return 0;
}
