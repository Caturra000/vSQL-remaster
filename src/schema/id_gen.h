#ifndef VSQL_ID_GEN_H
#define VSQL_ID_GEN_H

class id_gen {
    long long id; // TODO 并发时改成atomic
public:
    id_gen(): id(0) { }
    long long next() { return id++; }
};

#endif //VSQL_ID_GEN_H
