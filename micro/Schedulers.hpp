#ifndef __SCHEDULERS_HPP__
#define __SCHEDULERS_HPP__

#include "Base.hpp"

namespace Systematic {
    class IExplorer {
    public:
        virtual void Begin(Graph * g) = 0;
        virtual bool Explore(std::vector<Vertex *> & outOrder) = 0;
        virtual void End() = 0;
        virtual ~IExplorer() { }
    };

    IExplorer * CreateDfsExplorer(bool sleepSet = true);
}

namespace RandomWalk {
    void Basic(Graph * g, random_engine & random, std::map<Vertex *, int> & outOrderMap, std::vector<Vertex *> & outOrder);
}

namespace Pos {
    void Basic(Graph * g, random_engine & random, std::map<Vertex *, int> & outOrderMap, std::vector<Vertex *> & outOrder);
    void DependencyBased(Graph * g, random_engine & random, std::map<Vertex *, int> & outOrderMap, std::vector<Vertex *> & outOrder);
}

namespace Misc {
    void Rapos(Graph * g, random_engine & random, std::map<Vertex *, int> & outOrderMap, std::vector<Vertex *> & outOrder);
}

#endif
