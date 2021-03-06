#ifndef CPUBFS_BIN_H
#define CPUBFS_BIN_H

#include "globalbfs.hh"

typedef int64_t vertexType;
typedef int64_t rowtyp;

class CPUBFS_bin : public GlobalBFS<CPUBFS_bin, uint64_t, uint64_t, DistMatrix2d<vertexType, rowtyp, true, 64, false>>
{
    const int64_t col64;
    const int64_t row64;

    uint64_t  *__restrict__ visited;
    uint64_t  *fq_out;
    uint64_t  *__restrict__ fq_in;

public:
    typedef DistMatrix2d<vertexType, rowtyp, true, 64, false> MatrixT;
    CPUBFS_bin(MatrixT &_store, int64_t verbosity);

    ~CPUBFS_bin();

    void reduce_fq_out(uint64_t *startaddr,
                       long insize);    //Global Reducer of the local outgoing frontier queues.  Have to be implemented by the children.
    void getOutgoingFQ(uint64_t *&startaddr, long &outsize);
    void setModOutgoingFQ(uint64_t *startaddr, long insize); //startaddr: 0, self modification
    void getOutgoingFQ(vertexType globalstart, long size, uint64_t *&startaddr, long &outsize);
    void setIncommingFQ(vertexType globalstart, long size, uint64_t *startaddr, long &insize_max);
    bool istheresomethingnew();           //to detect if finished
    void setStartVertex(const vertexType start);
    void runLocalBFS();
};

#endif // CPUBFS_BIN_H
