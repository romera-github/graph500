/**
 *
 * BFS algorithm implementation with CUDA support.
 * Matthias Hauck, 2013
 *
 *
 *
 */
#include "config.h" // Autotools
#include "mpi.h"
#include <cstring>
#include <assert.h>
#include <cmath>
#include <algorithm>
#include "validate/validate.h"
#include "generator/make_graph.h"
#include "distmatrix2d.hh"

#if __cplusplus > 199711L  //std c++11 is a requirement
#include <random>
#else
#error This library needs at least a C++11 compliant compiler
#endif

#ifdef _COMPRESSION
#define NOMINMAX
#include "compression/compressionfactory.hh"
#endif

/**
 * Children of GlobalBFS
 * Implement the BFS algorithm
 *
 *
 *
 */
#ifdef _CUDA
#include "cuda/cuda_bfs.h"
#elif defined _OPENCL
#include "opencl/OCLrunner.hh"
#include "opencl/opencl_bfs.h"
#else
#include "cpubfs_bin.h"
#endif

// vertexType, compressionType
#include "types_bfs.h"
#include "constants.hh"

using std::vector;
using std::knuth_b;
using std::uniform_int_distribution;
using std::find;
using std::sort;
using std::string;

struct statistic
{
    double min;
    double firstquartile;
    double thirdquartile;
    double median;
    double max;
    double mean;
    double stddev;
    double hmean;
    double hstddev;
};

enum GGen
{
    G500 = 0,
    OLD_G500
};

/**
 * Prototype definitions
 *
 *
 *
 *
 *
 */


void externalArgumentsIterate(int argc, char *const *argv, int64_t &scale, int64_t &edgefactor,
                              int64_t &num_of_iterations, int64_t &verbosity, int &R, int &C, bool &R_set, bool &C_set,
                              int &graph_gen, int &gpus, double &queue_sizing, string &compressionCodec, int &compressionThreshold);
void outputIterationStatistics(statistic &bfs_time_s, statistic &nedge_s, statistic &teps_s);
void outputIterationInstrumentedStatistics(statistic &valid_time_s, statistic &lbfs_time_s, statistic &lbfs_share_s,
        statistic &lqueue_time_s, statistic &lqueue_share_s, statistic &rest_time_s,
        statistic &rest_share_s, statistic &lrowcom_s, statistic &lrowcom_share_s,
        statistic &lcolcom_s, statistic &lcolcom_share_s, statistic &lpredlistred_s,
        statistic &lpredlistred_share_s);
vertexType generateStartNode(const int64_t &vertices, knuth_b &generator);
template <class T> statistic getStatistics(vector <T> &input);
void outputMatrixGenerationResults(int size, const int64_t &global_edges, double constr_time, long global_edges_wd);
void printStat(statistic &input, const char *name, bool harmonic);
void outputGeneralStatistics(const int64_t &scale, const int64_t &edgefactor, int size, bool valid,
                             double make_graph_time, int iterations);
void outputBfsRunIterationResults(double rtstart, double rtstop, double gmax_lexp, double gmax_lqueue,
                                  double gmax_rowcom, double gmax_colcom, double gmax_predlistred);
void externalArgumentsVerify(bool R_set, bool C_set, int size, int &R, int &C);

/**
 * main()
 *
 *
 *
 *
 *
 */

int main(int argc, char **argv)
{

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"

    int64_t scale = 21;
    int64_t edgefactor = 16;
    int64_t num_of_iterations = 64;
    int64_t verbosity = 1;
    int64_t number_of_edges;
    int64_t vertices;
    int64_t global_edges;
    double tstart, tstop;
    double rtstart, rtstop;
    double make_graph_time, constr_time;
    bool R_set = false, C_set = false, valid = true;
    int R, C, graph_gen = G500, size, rank;
    int level, this_valid;
    int next, maxiterations;
    long local_edges, elem, global_edges_wd;
    string compressionCodec = "";
    string predecessorListCompressionCodec = "";
    int compressionThreshold = 0;
    int iterations = 0, maxgenvtx =
                         32; // relative number of maximum attempts to find a valid start vertix per possible attempt
    vector <vertexType> tries;
    vector <double> bfs_time;
    vector <long> nedge; //number of edges
    vector <double> teps;
    int compressionCodecLength;
    char *compressionCodecBuffer = NULL;
    vertexType start, locstart, num_edges;

#ifdef _CUDA
    int gpus = 0;
    double queue_sizing = 1.20;
#endif

#ifdef INSTRUMENTED
    double lexp, lqueue, rowcom, colcom, predlistred;
    double gmax_lexp, gmax_lqueue, gmax_rowcom, gmax_colcom, gmax_predlistred;
    vector<double> valid_time;
    vector<double> bfs_local;
    vector<double> bfs_local_share;
    vector<double> queue_local;
    vector<double> queue_local_share;
    vector<double> rest;
    vector<double> rest_share;
    vector<double> lrowcom;
    vector<double> lrowcom_share;
    vector<double> lcolcom;
    vector<double> lcolcom_share;
    vector<double> lpredlistred;
    vector<double> lpredlistred_share;
#endif

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

   if (rank == 0)
    {
        externalArgumentsIterate(argc, argv, scale, edgefactor, num_of_iterations, verbosity,
                                 R, C, R_set, C_set, graph_gen,
                                 gpus, queue_sizing, compressionCodec, compressionThreshold);
        externalArgumentsVerify(R_set, C_set, size, R, C);
        printf("row slices: %d, column slices: %d\n", R, C);
        compressionCodecLength = compressionCodec.size() + 1;
        compressionCodecBuffer = new char[compressionCodecLength];
        strncpy(compressionCodecBuffer, compressionCodec.c_str(), compressionCodecLength);
    }

    MPI_Bcast(&scale, 1, MPI_INT64_T, 0, MPI_COMM_WORLD);
    MPI_Bcast(&edgefactor, 1, MPI_INT64_T, 0, MPI_COMM_WORLD);
    MPI_Bcast(&num_of_iterations, 1, MPI_INT64_T, 0, MPI_COMM_WORLD);
    MPI_Bcast(&R, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&C, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&compressionThreshold, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&compressionCodecLength, 1, MPI_INT, 0, MPI_COMM_WORLD);
    if (rank != 0)
    {
        compressionCodecBuffer = new char[compressionCodecLength];
    }
    MPI_Bcast(compressionCodecBuffer, compressionCodecLength, MPI_CHAR, 0, MPI_COMM_WORLD);
    compressionCodec = compressionCodecBuffer;

#ifdef _CUDA
    MPI_Bcast(&gpus      , 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&queue_sizing, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
#endif

    MPI_Bcast(&verbosity, 1, MPI_INT64_T, 0, MPI_COMM_WORLD);
    // Close unnecessary nodes
    if (R * C != size)
    {
        printf("Number of nodes and size of grid do not match.\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    //new number of vertices
    vertices = 1L << scale;

    // Graph generation
    MPI_Barrier(MPI_COMM_WORLD);
    tstart = MPI_Wtime();
    packed_edge *edgelist = 0;
    make_graph(scale, edgefactor << scale, 1, 2, &number_of_edges, &edgelist);
    MPI_Barrier(MPI_COMM_WORLD);
    tstop = MPI_Wtime();

    MPI_Reduce(&number_of_edges, &global_edges_wd, 1, MPI_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
    make_graph_time = tstop - tstart;
    if (rank == 0)
    {
        printf("Input list of edges genereted.\n");
        printf("%e edge(s) generated in %fs (%f Medges/s on %d processor(s))\n", static_cast<double>(global_edges_wd),
               make_graph_time, global_edges_wd / make_graph_time * 1.e-6, size);
    }

    MPI_Barrier(MPI_COMM_WORLD);
    tstart = MPI_Wtime();

    // Matrix definition
#ifdef _OPENCL
    typedef OpenCL_BFS::MatrixT MatrixT;
#elif defined _CUDA
    typedef CUDA_BFS::MatrixT MatrixT;
#else
    typedef CPUBFS_bin::MatrixT MatrixT;
#endif

    // Matrix generation
    MatrixT store(R, C);
    store.setupMatrix2(edgelist, number_of_edges);
    if ((verbosity >= 1) && (rank == 0))
    {
        printf("Global matrix redistribution done!\n");
    }

#if defined(_COMPRESSION) && defined(_SIMD)
#if defined(_COMPRESSIONDEBUG)
    // Check Matrix. Values lower than 32 bits are needed for SIMDcompression
    if (rank == 0)
    {
        printf("Check matrix values (lower than 2^32)...");
    }
    if (!store.allValuesSmallerThan32Bits())
    {
        printf("error. Not all values in the graph are lower than 2^32\n");
        MPI_Finalize();
        exit(1);
    }
    if (rank == 0)
    {
        printf(" done!\n");
    }
#endif
#endif

#if defined(_COMPRESSION) // && defined(_SIMD_PLUS)
#if defined(_COMPRESSIONDEBUG)
    // Check Matrix. Values are possitive
    if (rank == 0)
    {
        printf("Check matrix values (positive)...");
    }
    if (!store.allValuesPositive())
    {
        printf("debug. Not all values in the graph are positive.\n");
        //MPI_Finalize();
        //exit(1);
    }
    if (rank == 0)
    {
        printf(" done!\n");
    }
#endif
#endif


#ifdef _COMPRESSION
    /**
     * @values "nocompression", "cpusimd", "gpusimt"
     */
#if defined(_SIMD)
    Compression<vertexType, compressionType> &schema = *CompressionFactory<vertexType, compressionType>::getFromName("cpusimd");
    schema.reconfigure(compressionThreshold, compressionCodec);
    // non ordered 64-bit integer data. choosed VSimple codec
    Compression<vertexType, compressionType> &predecessorListSchema = *CompressionFactory<vertexType, compressionType>::getFromName("cpusimd");
    predecessorListCompressionCodec = "maskedvbyte";
    predecessorListSchema.reconfigure(compressionThreshold, predecessorListCompressionCodec);
#elif defined(_SIMD_PLUS)
    Compression<vertexType, compressionType> &schema = *CompressionFactory<vertexType, compressionType>::getFromName("simdplus");
#elif defined(_SIMT)
    Compression<vertexType, compressionType> &schema = *CompressionFactory<vertexType, compressionType>::getFromName("gpusimt");
#else
    Compression<vertexType, compressionType> &schema = *CompressionFactory<vertexType, compressionType>::getFromName("nocompression");
#endif
#endif

#ifdef _OPENCL
    OCLRunner oclrun;
    OpenCL_BFS bfs(store, *oclrun);
#elif defined _CUDA
    CUDA_BFS bfs(store, gpus, queue_sizing, verbosity);
#else
    CPUBFS_bin bfs(store, verbosity, rank);
#endif

    tstop = MPI_Wtime();
    local_edges = store.getEdgeCount();
    MPI_Reduce(&local_edges, &global_edges, 1, MPI_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
    constr_time = tstop - tstart;
    if (rank == 0)
    {
        outputMatrixGenerationResults(size, global_edges, constr_time, global_edges_wd);
    }

#ifdef INSTRUMENTED
    if (verbosity >= 16)
    {
        // print matrix
        const rowtyp *rowp = store.getRowPointer();
        const vertexType *columnp = store.getColumnIndex();
        for (int i = 0; i < store.getLocRowLength(); ++i)
        {
            printf("%lX: ", static_cast<int64_t>(store.localtoglobalRow(i)));
            for (rowtyp j = rowp[i]; j < rowp[i + 1]; ++j)
            {
                printf("%lX ", static_cast<int64_t>(columnp[j]));
            }
            printf("\n");
        }
    }
#endif

    // random number generator
    knuth_b generator;
    uniform_int_distribution<vertexType> distribution(0, vertices - 1);

    maxiterations = num_of_iterations * maxgenvtx;

    if (rank == 0)
    {
        int giteration = 0; // number of tried iterations
        while (iterations < num_of_iterations && giteration < maxiterations)
        {
            ++giteration;
            start = distribution(generator);

            // generate start node
            //skip already visited
            if (find(tries.begin(), tries.end(), start) != tries.end())
            {
                continue;
            }
            // tell other nodes that there is another vertex to check
            next = 1;
            MPI_Bcast(&next, 1, MPI_INT, 0, MPI_COMM_WORLD);
            // test if vertex has edges to other vertices
            elem = 0;
            MPI_Bcast(&start, 1, MPI_LONG, 0, MPI_COMM_WORLD);
            if (store.isLocalRow(start))
            {
                vertexType locstart = store.globaltolocalRow(start);
                elem = store.getRowPointer()[locstart + 1] - store.getRowPointer()[locstart];
            }
            MPI_Reduce(MPI_IN_PLACE, &elem, 1, MPI_LONG, MPI_SUM, 0, MPI_COMM_WORLD);

            if (elem > 0)
            {
                tries.push_back(start);
                ++iterations;
            }
        }
        next = 0;
        MPI_Bcast(&next, 1, MPI_INT, 0, MPI_COMM_WORLD);
    }
    else
    {
        while (true)
        {
            MPI_Bcast(&next, 1, MPI_INT, 0, MPI_COMM_WORLD);
            if (next == 0)
            {
                break;
            }
            elem = 0;
            MPI_Bcast(&start, 1, MPI_LONG, 0, MPI_COMM_WORLD);
            if (store.isLocalRow(start))
            {
                locstart = store.globaltolocalRow(start);
                elem = store.getRowPointer()[locstart + 1] - store.getRowPointer()[locstart];
            }
            MPI_Reduce(&elem, &elem, 1, MPI_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
        }
    }

    // BFS runs start
    MPI_Bcast(&iterations, 1, MPI_INT, 0, MPI_COMM_WORLD);

    for (int i = 0; i < iterations; ++i)
    {
        // BFS
        MPI_Barrier(MPI_COMM_WORLD);
        rtstart = MPI_Wtime();
#ifdef _COMPRESSION
#ifdef INSTRUMENTED
        if (rank == 0)
        {
            bfs.runBFS(tries[i], lexp, lqueue, rowcom, colcom, predlistred, schema, predecessorListSchema);
        }
        else
        {
            bfs.runBFS(-1, lexp, lqueue, rowcom, colcom, predlistred, schema, predecessorListSchema);
        }
#else
        if (rank == 0)
        {
            bfs.runBFS(tries[i], schema, predecessorListSchema);
        }
        else
        {
            bfs.runBFS(-1, schema, predecessorListSchema);
        }
#endif
#else
#ifdef INSTRUMENTED
        if (rank == 0)
        {
            bfs.runBFS(tries[i], lexp, lqueue, rowcom, colcom, predlistred);
        }
        else
        {
            bfs.runBFS(-1, lexp, lqueue, rowcom, colcom, predlistred);
        }
#else
        if (rank == 0)
        {
            bfs.runBFS(tries[i]);
        }
        else
        {
            bfs.runBFS(-1);
        }
#endif
#endif
        rtstop = MPI_Wtime();

#ifdef INSTRUMENTED
        MPI_Reduce(&lexp, &gmax_lexp, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
        MPI_Reduce(&lqueue, &gmax_lqueue, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
        MPI_Reduce(&rowcom, &gmax_rowcom, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
        MPI_Reduce(&colcom, &gmax_colcom, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
        MPI_Reduce(&predlistred, &gmax_predlistred, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
#endif
        if (rank == 0)
        {
            if (verbosity >= 1)
            {
                printf("BFS Iteration %d: Finished in %fs\n", i, (rtstop - rtstart));
            }
#ifdef INSTRUMENTED
            if (verbosity >= 1)
            {
                outputBfsRunIterationResults(rtstart, rtstop, gmax_lexp, gmax_lqueue, gmax_rowcom, gmax_colcom,
                                             gmax_predlistred);
            }

            bfs_local.push_back(gmax_lexp);
            bfs_local_share.push_back(gmax_lexp / (rtstop - rtstart));
            queue_local.push_back(gmax_lqueue);
            queue_local_share.push_back(gmax_lqueue / (rtstop - rtstart));
            rest.push_back((rtstop - rtstart) - gmax_lexp - gmax_lqueue);
            rest_share.push_back(1. - (gmax_lexp + gmax_lqueue) / (rtstop - rtstart));

            lrowcom.push_back(gmax_rowcom);
            lrowcom_share.push_back(gmax_rowcom / (rtstop - rtstart));
            lcolcom.push_back(gmax_colcom);
            lcolcom_share.push_back(gmax_colcom / (rtstop - rtstart));
            lpredlistred.push_back(gmax_predlistred);
            lpredlistred_share.push_back(gmax_predlistred / (rtstop - rtstart));
#endif

        }
        // Validation
        tstart = MPI_Wtime();
        if (rank == 0)
        {
            start = tries[i];
            MPI_Bcast(&start, 1, MPI_LONG, 0, MPI_COMM_WORLD);
        }
        else
        {
            MPI_Bcast(&start, 1, MPI_LONG, 0, MPI_COMM_WORLD);
        }
        this_valid = validate_bfs_result<MatrixT>(store, edgelist, number_of_edges,
                     vertices, static_cast<int64_t>(start), bfs.getPredecessor(),
                     &num_edges, &level);
        tstop = MPI_Wtime();
        if (rank == 0)
        {
            printf("Validation of iteration %d finished in %fs\n", i, (tstop - tstart));
        }
        valid = valid && this_valid;

        //print and save statistic
        if (rank == 0)
        {
            printf("Result: %s %ld Edge(s) processed, %f MTeps\n", (this_valid) ? "Valid" : "Invalid",
                   static_cast<long>(num_edges), num_edges / (rtstop - rtstart) * 1E-6);
            //for statistic
            bfs_time.push_back(rtstop - rtstart);
            nedge.push_back(num_edges);
            teps.push_back(num_edges / (rtstop - rtstart));

#ifdef INSTRUMENTED
            valid_time.push_back(tstop - tstart);
#endif
        }
    } // BFS runs end
    free(edgelist);
    // Output statistics
    if (rank == 0)
    {
        outputGeneralStatistics(scale, edgefactor, size, valid, make_graph_time, iterations);
#ifdef _CUDA
        printf("gpus_per_process: %d\n", gpus);
        printf("total_gpus: %d\n", gpus * size);
#endif
        printf("construction_time: %2.3e\n", constr_time);
        statistic bfs_time_s = getStatistics(bfs_time);
        statistic nedge_s = getStatistics(nedge);
        statistic teps_s = getStatistics(teps);
        outputIterationStatistics(bfs_time_s, nedge_s, teps_s);
#ifdef INSTRUMENTED
        statistic valid_time_s = getStatistics(valid_time);
        statistic lbfs_time_s = getStatistics(bfs_local);
        statistic lbfs_share_s = getStatistics(bfs_local_share);
        statistic lqueue_time_s = getStatistics(queue_local);
        statistic lqueue_share_s = getStatistics(queue_local_share);
        statistic rest_time_s = getStatistics(rest);
        statistic rest_share_s = getStatistics(rest_share);
        statistic lrowcom_s = getStatistics(lrowcom);
        statistic lrowcom_share_s = getStatistics(lrowcom_share);
        statistic lcolcom_s = getStatistics(lcolcom);
        statistic lcolcom_share_s = getStatistics(lcolcom_share);
        statistic lpredlistred_s = getStatistics(lpredlistred);
        statistic lpredlistred_share_s = getStatistics(lpredlistred_share);
        outputIterationInstrumentedStatistics(valid_time_s, lbfs_time_s, lbfs_share_s, lqueue_time_s,
                                              lqueue_share_s, rest_time_s, rest_share_s, lrowcom_s,
                                              lrowcom_share_s, lcolcom_s, lcolcom_share_s, lpredlistred_s,
                                              lpredlistred_share_s);
#endif

    }
    delete[] compressionCodecBuffer;
    MPI_Finalize();

#pragma GCC diagnostic pop
}



/**
 * Auxiliar functions implementations
 *
 *
 *
 *
 *
 */

void outputBfsRunIterationResults(double rtstart, double rtstop, double gmax_lexp, double gmax_lqueue,
                                  double gmax_rowcom, double gmax_colcom, double gmax_predlistred)
{
    printf("max. local exp.:     %fs(%f%%)\n", gmax_lexp,  100.*gmax_lexp / (rtstop - rtstart));
    printf("max. queue handling: %fs(%f%%)\n", gmax_lqueue, 100.*gmax_lqueue / (rtstop - rtstart));
    printf("est. rest:           %fs(%f%%)\n", (rtstop - rtstart) - gmax_lexp - gmax_lqueue,
           100.*(1. - (gmax_lexp + gmax_lqueue) / (rtstop - rtstart)));
    printf("max. row com.:       %fs(%f%%)\n", gmax_rowcom,  100.*gmax_rowcom / (rtstop - rtstart));
    printf("max. col com.:       %fs(%f%%)\n", gmax_colcom,  100.*gmax_colcom / (rtstop - rtstart));
    printf("max. pred. list. red:%fs(%f%%)\n", gmax_predlistred,  100.*gmax_predlistred / (rtstop - rtstart));
}

void outputGeneralStatistics(const int64_t &scale, const int64_t &edgefactor, int size, bool valid,
                             double make_graph_time, int iterations)
{
    printf("Validation: %s\n", (valid) ? "passed" : "failed!");
    printf("SCALE: %ld\n", scale);
    printf("edgefactor: %ld\n", edgefactor);
    printf("NBFS: %d\n", iterations);
    printf("graph_generation: %2.3e\n", make_graph_time);
    printf("num_mpi_processes: %d\n", size);

}

void outputMatrixGenerationResults(int size, const int64_t &global_edges, double constr_time, long global_edges_wd)
{
    printf("Adjacency Matrix setup.\n");
    printf("%e edge(s) removed, because they are duplicates or self loops.\n",
           static_cast<double>(global_edges_wd - global_edges / 2));
    printf("%e unique edge(s) processed in %fs (%f Medges/s on %d processor(s))\n",
           static_cast<double>(global_edges), constr_time, global_edges / constr_time * 1.e-6, size);
}

void outputIterationInstrumentedStatistics(statistic &valid_time_s, statistic &lbfs_time_s, statistic &lbfs_share_s,
        statistic &lqueue_time_s, statistic &lqueue_share_s, statistic &rest_time_s,
        statistic &rest_share_s, statistic &lrowcom_s, statistic &lrowcom_share_s,
        statistic &lcolcom_s, statistic &lcolcom_share_s, statistic &lpredlistred_s,
        statistic &lpredlistred_share_s)
{

    printStat(valid_time_s, "validation_time", false);
    printStat(lbfs_time_s, "local_bfs_time", false);
    printStat(lbfs_share_s, "bfs_local_share", true);
    printStat(lqueue_time_s, "local_queue_time", false);
    printStat(lqueue_share_s, "queue_local_share", true);
    printStat(rest_time_s, "rest_time", false);
    printStat(rest_share_s, "rest_share", true);
    printStat(lrowcom_s, "row_com_time", false);
    printStat(lrowcom_share_s, "row_com_share", true);
    printStat(lcolcom_s, "column_com_time", false);
    printStat(lcolcom_share_s, "column_com_share", true);
    printStat(lpredlistred_s, "predecessor_list_reduction_time", false);
    printStat(lpredlistred_share_s, "predecessor_list_reduction_share", true);
}

void outputIterationStatistics(statistic &bfs_time_s, statistic &nedge_s, statistic &teps_s)
{
    printStat(bfs_time_s, "time", false);
    printStat(nedge_s, "nedge", false);
    printStat(teps_s, "TEPS", true);
}

void externalArgumentsVerify(bool R_set, bool C_set, int size, int &R, int &C)
{
    if (R_set && !C_set)
    {
        if (R > size)
        {
            printf("Error not enought nodesRequested: %d available: %d\n", R, size);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        else
        {
            C = size / R;
        }
    }
    else if (!R_set && C_set)
    {
        if (C > size)
        {
            printf("Error not enought nodes. Requested: %d available: %d\n", C, size);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        else
        {
            R = size / C;
        }
    }
    else
    {
        R = size;
        C = 1;
    }
}

void externalArgumentsIterate(int argc, char *const *argv, int64_t &scale, int64_t &edgefactor,
                              int64_t &num_of_iterations, int64_t &verbosity, int &R, int &C, bool &R_set, bool &C_set,
                              int &graph_gen, int &gpus, double &queue_sizing, string &compressionCodec, int &compressionThreshold)
{
    int i = 0;
    while (i < argc)
    {
        if (!strcmp(argv[i], "-s"))
        {
            if (i + 1 < argc)
            {
                int s_tmp = atol(argv[i + 1]);
                if (s_tmp < 1)
                {
                    printf("Invalid scale factor: %s\n", argv[i + 1]);
                }
                else
                {
                    scale = s_tmp;
                    ++i;
                }
            }
        }
        else if (!strcmp(argv[i], "-e"))
        {
            if (i + 1 < argc)
            {
                int e_tmp = atol(argv[i + 1]);
                if (e_tmp < 1)
                {
                    printf("Invalid edge factor: %s\n", argv[i + 1]);
                }
                else
                {
                    edgefactor = e_tmp;
                    ++i;
                }
            }
        }
        else if (!strcmp(argv[i], "-i"))
        {
            if (i + 1 < argc)
            {
                int i_tmp = atol(argv[i + 1]);
                if (i_tmp < 1)
                {
                    printf("Invalid number of iterations: %s\n", argv[i + 1]);
                }
                else
                {
                    num_of_iterations = i_tmp;
                    ++i;
                }
            }
        }
        else if (!strcmp(argv[i], "-R"))
        {
            if (i + 1 < argc)
            {
                int R_tmp = atoi(argv[i + 1]);
                if (R_tmp < 1)
                {
                    printf("Invalid row slice number: %s\n", argv[i + 1]);
                }
                else
                {
                    R_set = true;
                    R = R_tmp;
                    ++i;
                }
            }
        }
        else if (!strcmp(argv[i], "-C"))
        {
            if (i + 1 < argc)
            {
                int C_tmp = atoi(argv[i + 1]);
                if (C_tmp < 1)
                {
                    printf("Invalid column slice number: %s\n", argv[i + 1]);
                }
                else
                {
                    C_set = true;
                    C = C_tmp;
                    ++i;
                }
            }

#ifdef _CUDA
        }
        else if (!strcmp(argv[i], "-gpus"))
        {
            if (i + 1 < argc)
            {
                int gpus_tmp = atoi(argv[i + 1]);
                if (gpus_tmp < 1 || gpus_tmp > 8)
                {
                    printf("Invalid gpu number: %s\n", argv[i + 1]);
                }
                else
                {
                    gpus = gpus_tmp;
                    ++i;
                }
            }
        }
        else if (!strcmp(argv[i], "-qs"))
        {
            if (i + 1 < argc)
            {
                double qs_tmp = atof(argv[i + 1]);
                if (qs_tmp < 1.)
                {
                    printf("Invalid queue sizing: %s\n", argv[i + 1]);
                }
                else
                {
                    queue_sizing = qs_tmp;
                    ++i;
                }
            }
#endif
        }
        else if (!strcmp(argv[i], "-v"))
        {
            /* Verbosity level:
             * 0: Suppress all unnessesary output
             * 1: Level infos
             * 8: problem info
             * 16: Output Matrix
             * 24: problem pointer
             */
            if (i + 1 < argc)
            {
                long verbosity_tmp = atol(argv[i + 1]);
                if (verbosity_tmp < 0)
                {
                    printf("Invalid verbosity: %s\n", argv[i + 1]);
                }
                else
                {
                    verbosity = verbosity_tmp;
                    ++i;
                }
            }
        }
        else if (!strcmp(argv[i], "-g"))
        {
            // graph genarator
            if (i + 1 < argc)
            {
                if (!strcmp(argv[i + 1], "g500"))
                {
                    graph_gen = G500;
                    ++i;
                }
                else if (!strcmp(argv[i + 1], "old_g500"))
                {
                    graph_gen = OLD_G500;
                    ++i;
                }
                else
                {
                    printf("Generator %s unknown!\n", argv[i + 1]);
                    ++i;
                }
            }
        }
        else if (!strcmp(argv[i], "-btr"))
        {
            /**
             * compression-threshold value for rows
             */
            if (i + 1 < argc)
            {
                int compressionThreshold_tmp = atoi(argv[i + 1]);
                if (compressionThreshold_tmp < 1 || compressionThreshold_tmp > 0xffffff)
                {
                    printf("Invalid row compression-threshold value: %s\n", argv[i + 1]);
                }
                else
                {
                    compressionThreshold = compressionThreshold_tmp;
                    ++i;
                }
            }
        }
        else if (!strcmp(argv[i], "-be"))
        {
            /**
             * Compression extra string (e.g used for SIMDcomp codec string)
             */
            if (i + 1 < argc)
            {
                string compressionCodec_tmp = argv[i + 1];
                unsigned int extraArgLength = compressionCodec_tmp.length();
                if (extraArgLength > 128)
                {
                    printf("Invalid compression extra argument (required string): %s\n", argv[i + 1]);
                }
                else
                {
                    compressionCodec = compressionCodec_tmp;
                    ++i;
                }
            }
        }
        ++i;
    }
}

void printStat(statistic &input, const char *name, bool harmonic)
{
    printf("min_%s: %2.3e\n", name, input.min);
    printf("firstquartile_%s: %2.3e\n", name, input.firstquartile);
    printf("median_%s: %2.3e\n", name, input.median);
    printf("thirdquartile_%s: %2.3e\n", name, input.thirdquartile);
    printf("max_%s: %2.3e\n", name, input.max);

    if (harmonic)
    {
        printf("harmonic_mean_%s: %2.3e\n", name, input.hmean);
        printf("harmonic_stddev_%s: %2.3e\n", name, input.hstddev);
    }
    else
    {
        printf("mean_%s: %2.3e\n", name, input.mean);
        printf("stddev_%s: %2.3e\n", name, input.stddev);
    }
}

template<class T>
statistic getStatistics(vector <T> &input)
{
    statistic out;
    sort(input.begin(), input.end());
    out.min = static_cast<double>(input.front());
    if (1 * input.size() % 4 == 0)
        out.firstquartile = 0.5 * (input[1 * input.size() / 4 - 1] + input[1 * input.size() / 4]);
    else
        out.firstquartile = static_cast<double>(input[1 * input.size() / 4]);
    if (2 * input.size() % 4 == 0)
        out.median = 0.5 * (input[2 * input.size() / 4 - 1] + input[2 * input.size() / 4]);
    else
        out.median = static_cast<double>(input[2 * input.size() / 4]);
    if (3 * input.size() % 4 == 0)
        out.thirdquartile = 0.5 * (input[3 * input.size() / 4 - 1] + input[3 * input.size() / 4]);
    else
        out.thirdquartile = static_cast<double>(input[3 * input.size() / 4]);
    out.max = static_cast<double>(input.back());
    double qsum = 0.0, sum = 0.0;
    for (typename vector<T>::iterator it = input.begin(); it != input.end(); it++)
    {
        double it_val = static_cast<double>(*it);
        sum += it_val;
        qsum += it_val * it_val;
    }
    out.mean = sum / static_cast<double>(input.size());
    out.stddev = sqrt((qsum - sum * sum / static_cast<double>(input.size())) /
                      (static_cast<double>(input.size()) - 1));
    double iv_sum = 0;
    for (typename vector<T>::iterator it = input.begin(); it != input.end(); it++)
    {
        double it_val = static_cast<double>(*it);
        iv_sum += 1 / it_val;
    }
    out.hmean = static_cast<double>(input.size()) / iv_sum;
    // Harmonic standard deviation from:
    // Nilan Norris, The Standard Errors of the Geometric and Harmonic
    // Means and Their Application to Index Numbers, 1940.
    // http://www.jstor.org/stable/2235723
    double qiv_dif = 0.;
    for (typename vector<T>::iterator it = input.begin(); it != input.end(); it++)
    {
        double it_val = 1 / static_cast<double>(*it) - 1 / out.hmean;
        qiv_dif += it_val * it_val;
    }
    out.hstddev = (sqrt(qiv_dif) / (static_cast<double>(input.size()) - 1)) * out.hmean * out.hmean;
    return out;
}

