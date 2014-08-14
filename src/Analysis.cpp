#include "Analysis.hpp"

using namespace std;

vector<int> *Analysis::commonDOFs(vector<int> *e1, vector<int> *e2)
{
    vector<int> *output = new vector<int>();
    for (int i=0; i<e1->size(); ++i) {
        for (int j=0; j<e2->size(); ++j) {
            if (e1->at(i) == e2->at(j)) {
                output->push_back(e1->at(i));
            }
        }
    }
    return output;
}

vector<int> *Analysis::nodeAnaliser(Node *node, vector<int> *parent)
{
    auto getAllDOFs = [] (Node *n) {
        vector<int> *dofs = new vector<int>();
        for (Element *e : n->getElements()) {
            for (int dof : e->dofs) {
                if (std::find(dofs->cbegin(), dofs->cend(), dof) == dofs->cend()) {
                    dofs->push_back(dof);
                }
            }
        }
        return dofs;
    };

    printf("Analise: %d\n", node->getId());

    vector<int> *common;

    if (node->getLeft() != NULL && node->getRight() != NULL) {
        vector<int> *lDofs = getAllDOFs(node->getLeft());
        vector<int> *rDofs = getAllDOFs(node->getRight());

        printf("LDOFS: ");
        for (auto i : *lDofs) {
            printf("%d ", i);
        }
        printf("\n");

        printf("RDOFS: ");
        for (auto i : *rDofs) {
            printf("%d ", i);
        }
        printf("\n");


        common = Analysis::commonDOFs(lDofs, rDofs);
        free(lDofs);
        free(rDofs);
    }

    for (auto p = parent->cbegin(); p!=parent->cend(); ++p) {
        if (std::find(common->cbegin(), common->cend(), *p) == common->cend()) {
            common->push_back(*p);
        }
    }
    return common;
}

void Analysis::doAnalise(Mesh *mesh)
{
    Node *root = mesh->getRootNode();
}

tuple<edge, int> Analysis::parentEdge(edge e,
                                          std::map<int, std::map<vertex, int> > &levelVertices,
                                          std::map<int, std::map<edge, int> > &levelEdges,
                                          int level)
{
    vertex &v1 = std::get<0>(e);
    vertex &v2 = std::get<1>(e);
    int x1 = std::get<0>(v1);
    int y1 = std::get<1>(v1);
    int x2 = std::get<0>(v2);
    int y2 = std::get<1>(v2);

    // horizontal edge
    if (y1 == y2) {
        if (levelEdges[level-1].count(edge(v1,vertex(2*x2-x1, y1)))) {
            return tuple<edge, bool>(edge(v1,vertex(2*x2-x1, y1)), 2);
        }
        if (levelEdges[level-1].count(edge(vertex(2*x1-x2, y1), v2))) {
            return tuple<edge, bool>(edge(vertex(2*x1-x2, y1), v2), 1);
        }
        return tuple<edge, bool>(e, 0);
    } else {
        if (levelEdges[level-1].count(edge(v1,vertex(x1, 2*y2-y1)))) {
            return tuple<edge, bool>(edge(v1,vertex(x1, 2*y2-y1)), 2);
        }
        if (levelEdges[level-1].count(edge(vertex(x1, 2*y1-y2), v2))) {
            return tuple<edge, bool>(edge(vertex(x1, 2*y1-y2), v2), 1);
        }
        return tuple<edge, bool>(e, 0);
    }
}

void Analysis::enumerateElem(Mesh *mesh, Element *elem,
                             std::map<int, std::map<vertex, int> > &levelVertices,
                             std::map<int, std::map<edge, int> > &levelEdges,
                             int &n, int level)
{
    map<vertex, int> &vertices = levelVertices[level];
    map<edge, int> &edges = levelEdges[level];

    edge e1(vertex(elem->x1, elem->y1), vertex(elem->x2, elem->y1));
    edge e2(vertex(elem->x2, elem->y1), vertex(elem->x2, elem->y2));
    edge e3(vertex(elem->x1, elem->y2), vertex(elem->x2, elem->y2));
    edge e4(vertex(elem->x1, elem->y1), vertex(elem->x1, elem->y2));

    // for other layers we need to take into consideration also
    // h-adaptation and its influence on positions of DOF
    map<vertex, int> &parentVertices = levelVertices[level-1];
    map<edge, int> &parentEdges = levelEdges[level-1];

    tuple<edge, bool> ve1 = Analysis::parentEdge(e1, levelVertices, levelEdges, level);
    tuple<edge, bool> ve2 = Analysis::parentEdge(e2, levelVertices, levelEdges, level);
    tuple<edge, bool> ve3 = Analysis::parentEdge(e3, levelVertices, levelEdges, level);
    tuple<edge, bool> ve4 = Analysis::parentEdge(e4, levelVertices, levelEdges, level);


    vertex v1(std::min(std::get<0>(std::get<0>(std::get<0>(ve1))), std::get<0>(std::get<0>(std::get<0>(ve4)))),
              std::min(std::get<1>(std::get<0>(std::get<0>(ve1))), std::get<1>(std::get<0>(std::get<0>(ve4)))));
    vertex v2(std::max(std::get<0>(std::get<1>(std::get<0>(ve1))), std::get<0>(std::get<0>(std::get<0>(ve2)))),
              std::min(std::get<1>(std::get<1>(std::get<0>(ve1))), std::get<1>(std::get<0>(std::get<0>(ve2)))));
    vertex v3(std::max(std::get<0>(std::get<1>(std::get<0>(ve2))), std::get<0>(std::get<1>(std::get<0>(ve3)))),
              std::max(std::get<1>(std::get<1>(std::get<0>(ve2))), std::get<1>(std::get<1>(std::get<0>(ve3)))));
    vertex v4(std::min(std::get<0>(std::get<0>(std::get<0>(ve3))), std::get<0>(std::get<1>(std::get<0>(ve4)))),
              std::max(std::get<1>(std::get<0>(std::get<0>(ve3))), std::get<1>(std::get<1>(std::get<0>(ve4)))));

    auto add_vertex = [&] (vertex &v) {
        if (parentVertices.count(v)) {
            vertices[v] = parentVertices[v];
        } else {
            if (!vertices.count(v)) {
                vertices[v] = n++;
            }
        }
        elem->dofs.push_back(vertices[v]);
    };

    auto add_edge = [&] (edge &e) {
        if (parentEdges.count(e)) {
            edges[e] = parentEdges[e];
        } else {
            if (!edges.count(e)) {
                edges[e] = n;
                n += mesh->getPolynomial()-1;
            }
        }
        for (int i=0; i<(mesh->getPolynomial()-1); ++i) {
            elem->dofs.push_back(edges[e]+i);
        }
    };

    add_vertex(v1);
    add_vertex(v2);
    add_vertex(v3);
    add_vertex(v4);

    add_edge(std::get<0>(ve1));
    add_edge(std::get<0>(ve2));
    add_edge(std::get<0>(ve3));
    add_edge(std::get<0>(ve4));

    for (int i=0; i<(mesh->getPolynomial()-1)*(mesh->getPolynomial()-1); ++i) {
        elem->dofs.push_back(n+i);
    }
    n += (mesh->getPolynomial()-1)*(mesh->getPolynomial()-1);
}

void Analysis::enumerateElem1(Mesh *mesh, Element *elem,
                             map<int, map<vertex, int>> &levelVertices,
                             map<int, map<edge, int>> &levelEdges,
                             int &n)
{
    map<vertex, int> &vertices = levelVertices[1];
    map<edge, int> &edges = levelEdges[1];

    vertex v1(elem->x1, elem->y1);
    vertex v2(elem->x2, elem->y1);
    vertex v3(elem->x2, elem->y2);
    vertex v4(elem->x1, elem->y2);

    edge e1(v1, v2);
    edge e2(v2, v3);
    edge e3(v4, v3);
    edge e4(v1, v4);

    auto add_vertex = [&] (vertex &v) {
        if (!vertices.count(v)) {
            vertices[v] = n++;
        }
        elem->dofs.push_back(vertices[v]);
    };

    auto add_edge = [&] (edge &e) {
        if (!edges.count(e)) {
            edges[e] = n;
            n += mesh->getPolynomial()-1;
        }
        for (int i=0; i<mesh->getPolynomial()-1; ++i) {
            elem->dofs.push_back(edges[e]+i);
        }
    };

    // vertices
    add_vertex(v1);
    add_vertex(v2);
    add_vertex(v3);
    add_vertex(v4);

    // edges
    add_edge(e1);
    add_edge(e2);
    add_edge(e3);
    add_edge(e4);
    // in 2-dimensional space the faces do not overlap
    for (int i=0; i<(mesh->getPolynomial()-1)*(mesh->getPolynomial()-1); ++i) {
        elem->dofs.push_back(n+i);
    }
    n += (mesh->getPolynomial()-1)*(mesh->getPolynomial()-1);
}

void Analysis::enumerateDOF(Mesh *mesh)
{
    map<int, vector<Element*>> elementMap;
    set<int> levels;

    map<int, map<vertex, int>> levelVertices;
    map<int, map<edge, int>> levelEdges;

    int n = 1;

    // now, we have level plan for mesh
    for (Element *e : mesh->getElements()) {
        levels.insert(e->k);
        elementMap[e->k].push_back(e);
    }

    // implementation assumes that the neighbours may vary on one level only
    for (int level : levels) {
        vector<Element *> elems = elementMap[level];
        // on the first layer we do not need to care about adaptation
        for (Element *elem : elems) {
            if (level == 1) {
                Analysis::enumerateElem1(mesh, elem, levelVertices, levelEdges, n);
            } else {
                Analysis::enumerateElem(mesh, elem, levelVertices, levelEdges, n, level);
            }
        }
    }
    mesh->setDofs(n-1);
}