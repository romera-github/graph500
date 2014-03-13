#include "distmatrix2d.hh"
#include <vector>
#include <cstdio>
#include <assert.h>

#ifndef GLOBALBFS_HH
#define GLOBALBFS_HH

/*
 * This classs implements a distributed level synchronus BFS on global scale.
 */
template<class Derived,class FQ_T=void, class STORE=DistMatrix2d<> >
class GlobalBFS
{
    MPI_Comm row_comm, col_comm;

    // sending node column slice, startvtx, size
    std::vector<typename STORE::fold_prop> fold_fq_props;

protected:
    const STORE& store;
    vtxtype* predessor;

    MPI_Datatype fq_tp_type; //Frontier Queue Transport Type
    FQ_T*   recv_fq_buff;
    long    recv_fq_buff_length;
    //virtual void reduce_fq_out(FQ_T* startaddr, long insize)=0;    //Global Reducer of the local outgoing frontier queues.  Have to be implemented by the children.
    //virtual void getOutgoingFQ(FQ_T* &startaddr, vtxtype& outsize)=0;
    //virtual void setModOutgoingFQ(FQ_T* startaddr, long insize)=0; //startaddr: 0, self modification
    //virtual void getOutgoingFQ(vtxtype globalstart, vtxtype size, FQ_T* &startaddr, vtxtype& outsize)=0;
    //virtual void setIncommingFQ(vtxtype globalstart, vtxtype size, FQ_T* startaddr, vtxtype& insize_max)=0;
    //virtual bool istheresomethingnew()=0;           //to detect if finished
    //virtual void setStartVertex(const vtxtype start)=0;
    //virtual void runLocalBFS()=0;


public:
    GlobalBFS(STORE& _store);

    #ifdef INSTRUMENTED
    void runBFS(vtxtype startVertex, double& lexp, double &lqueue);
    #else
    void runBFS(vtxtype startVertex);
    #endif


    vtxtype* getPredessor();
};

template<class Derived,class FQ_T,class STORE>
GlobalBFS<Derived,FQ_T,STORE>::GlobalBFS(STORE &_store): store(_store)
{
     // Split communicator into row and column communicator
     // Split by row, rank by column
     MPI_Comm_split(MPI_COMM_WORLD, store.getLocalRowID(), store.getLocalColumnID(), &row_comm);
     // Split by column, rank by row
     MPI_Comm_split(MPI_COMM_WORLD, store.getLocalColumnID(), store.getLocalRowID(), &col_comm);

     fold_fq_props = store.getFoldProperties();

}

/*
 * BFS search:
 * 0) Node 0 sends start vertex to all nodes
 * 1) Nodes test, if they are responsible for this vertex and push it,
 *    if they are in there fq
 * 2) Local expansion
 * 3) Test if anything is done
 * 4) global expansion
 * 5) global fold
*/
#ifdef INSTRUMENTED
template<class Derived,class FQ_T,class STORE>
void GlobalBFS<Derived,FQ_T,STORE>::runBFS(vtxtype startVertex, double& lexp, double& lqueue)
#else
template<class Derived,class FQ_T,class STORE>
void GlobalBFS<Derived,FQ_T,STORE>::runBFS(vtxtype startVertex)
#endif
{
    #ifdef INSTRUMENTED
    double tstart, tend;
    lexp =0;
    lqueue =0;
    #endif

// 0
    MPI_Bcast(&startVertex,1,MPI_LONG,0,MPI_COMM_WORLD);
// 1
    #ifdef INSTRUMENTED
    tstart = MPI_Wtime();
    #endif
    static_cast<Derived*>(this)->setStartVertex(startVertex);
    #ifdef INSTRUMENTED
    tend = MPI_Wtime();
    lqueue +=tend-tstart;
    #endif
// 2
    while(true){
    #ifdef INSTRUMENTED
    tstart = MPI_Wtime();
    #endif
    static_cast<Derived*>(this)->runLocalBFS();
    #ifdef INSTRUMENTED
    tend = MPI_Wtime();
    lexp +=tend-tstart;
    #endif
// 3
    int anynewnodes, anynewnodes_global;
    #ifdef INSTRUMENTED
    tstart = MPI_Wtime();
    #endif
    anynewnodes = static_cast<Derived*>(this)->istheresomethingnew();
    #ifdef INSTRUMENTED
    tend = MPI_Wtime();
    lqueue +=tend-tstart;
    #endif

    MPI_Allreduce(&anynewnodes, &anynewnodes_global,1,MPI_INT,MPI_LOR,MPI_COMM_WORLD);
    if(!anynewnodes_global){
        MPI_Allreduce(MPI_IN_PLACE, predessor ,store.getLocColLength(),MPI_LONG,MPI_MAX,col_comm);
        return; //There is nothing too do. Finish iteration.
    }
// 4
    // tree based reduce operation with messages of variable size
    // root 0
    int rounds = 0;
    while((1 << rounds) < store.getNumRowSl() ){
        if((store.getLocalRowID() >> rounds)%2 == 1){
            FQ_T* startaddr_fq;
            long _outsize;
            //comute recv addr
            int recv_addr = (store.getLocalRowID() + store.getNumRowSl() - (1 << rounds)) % store.getNumRowSl();
            //get fq to send
            #ifdef INSTRUMENTED
            tstart = MPI_Wtime();
            #endif
            static_cast<Derived*>(this)->getOutgoingFQ(startaddr_fq, _outsize);
            #ifdef INSTRUMENTED
            tend = MPI_Wtime();
            lqueue +=tend-tstart;
            #endif
            //send fq
            MPI_Ssend(startaddr_fq,_outsize ,fq_tp_type,recv_addr,rounds,col_comm);
            break;
        } else if ( store.getLocalRowID() + (1 << rounds) < store.getNumRowSl() ){
            MPI_Status    status;
            int count;
            //compute send addr
            int sender_addr = (store.getLocalRowID() +  (1 << rounds)) % store.getNumRowSl();
            //recv fq
            MPI_Recv(recv_fq_buff, recv_fq_buff_length, fq_tp_type,sender_addr,rounds, col_comm, &status);
            MPI_Get_count(&status, fq_tp_type, &count);
            //do reduce
            #ifdef INSTRUMENTED
            tstart = MPI_Wtime();
            #endif
            static_cast<Derived*>(this)->reduce_fq_out(recv_fq_buff,static_cast<long>(count));
            #ifdef INSTRUMENTED
            tend = MPI_Wtime();
            lqueue +=tend-tstart;
            #endif
        }
        rounds++;
    }

    //distribute solution
    if(0 == store.getLocalRowID())
    {
        FQ_T* startaddr_fq;
        long _outsize;
        //get fq to send
        #ifdef INSTRUMENTED
        tstart = MPI_Wtime();
        #endif
        static_cast<Derived*>(this)->getOutgoingFQ(startaddr_fq, _outsize);
        #ifdef INSTRUMENTED
        tend = MPI_Wtime();
        lqueue +=tend-tstart;
        #endif
        MPI_Bcast(&_outsize,1,MPI_LONG,0 ,col_comm);
        MPI_Bcast(startaddr_fq,_outsize,fq_tp_type,0 ,col_comm);
        #ifdef INSTRUMENTED
        tstart = MPI_Wtime();
        #endif
        static_cast<Derived*>(this)->setModOutgoingFQ(0,_outsize);
        #ifdef INSTRUMENTED
        tend = MPI_Wtime();
        lqueue +=tend-tstart;
        #endif
    } else {
        long _outsize;
        MPI_Bcast(&_outsize,1,MPI_LONG,0,col_comm);
        assert(_outsize <= recv_fq_buff_length);
        MPI_Bcast(recv_fq_buff,_outsize,fq_tp_type,0,col_comm);
        #ifdef INSTRUMENTED
        tstart = MPI_Wtime();
        #endif
        static_cast<Derived*>(this)->setModOutgoingFQ(recv_fq_buff,_outsize);
        #ifdef INSTRUMENTED
        tend = MPI_Wtime();
        lqueue +=tend-tstart;
        #endif
    }
// 5
    for(typename std::vector<typename STORE::fold_prop>::iterator it = fold_fq_props.begin(); it  != fold_fq_props.end(); it++){
        if(it->sendColSl == store.getLocalColumnID() ){
            FQ_T*   startaddr;
            long     outsize;
            #ifdef INSTRUMENTED
            tstart = MPI_Wtime();
            #endif
            static_cast<Derived*>(this)->getOutgoingFQ(it->startvtx, it->size, startaddr, outsize);
            #ifdef INSTRUMENTED
            tend = MPI_Wtime();
            lqueue +=tend-tstart;
            #endif
            MPI_Bcast(&outsize,1,MPI_LONG,it->sendColSl,row_comm);
            MPI_Bcast(startaddr,outsize,fq_tp_type,it->sendColSl,row_comm);
            #ifdef INSTRUMENTED
            tstart = MPI_Wtime();
            #endif
            static_cast<Derived*>(this)->setIncommingFQ(it->startvtx, it->size, startaddr, outsize);
            #ifdef INSTRUMENTED
            tend = MPI_Wtime();
            lqueue +=tend-tstart;
            #endif
        }else{
            long     outsize;
            MPI_Bcast(&outsize,1,MPI_LONG,it->sendColSl,row_comm);
            assert(outsize <= recv_fq_buff_length);
            MPI_Bcast(recv_fq_buff,outsize,fq_tp_type,it->sendColSl,row_comm);
            #ifdef INSTRUMENTED
            tstart = MPI_Wtime();
            #endif
            static_cast<Derived*>(this)->setIncommingFQ(it->startvtx, it->size, recv_fq_buff, outsize);
            #ifdef INSTRUMENTED
            tend = MPI_Wtime();
            lqueue +=tend-tstart;
            #endif
        }
    }
}
}

template<class Derived,class FQ_T,class STORE>
vtxtype* GlobalBFS<Derived,FQ_T,STORE>::getPredessor()
{
    return  predessor;
}

#endif // GLOBALBFS_HH
