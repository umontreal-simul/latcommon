//
//  main.cpp
//  Lattice Tester
//
//  Created by Erwan Bourceret on 18/04/2017.
//  Copyright © 2017 DIRO. All rights reserved.
//
/*
 * The purpose of this example is to compare the time and
 * the accuracy of each reduction method. We compute several
 * random matrix and apply algorithms. The parameters of this
 * analysis are defined at the beginning of this file,
 * after include part. It is possible to analyse the computing
 * time on a graph with the R distribution.
 */


/*
 * Select the output option among
 * [PRINT_CONSOLE, WITH_R].
 *
 */

#define PRINT_CONSOLE

// Include Header
#include <iostream>
#include <map>
#include <fstream>
#include <iterator>
#include <string>
#include <sstream>
#include <iomanip>
#include <time.h>

// Include LatticeTester Header
#include "latticetester/Util.h"
#include "latticetester/Const.h"
#include "latticetester/Types.h"
#include "latticetester/IntFactor.h"
#include "latticetester/IntLatticeBasis.h"
#include "latticetester/Reducer.h"
#include "latticetester/Types.h"

// Include NTL Header
#include <NTL/tools.h>
#include <NTL/ctools.h>
#include <NTL/ZZ.h>
#include <NTL/ZZ_p.h>
#include "NTL/vec_ZZ.h"
#include "NTL/vec_ZZ_p.h"
#include <NTL/vec_vec_ZZ.h>
#include <NTL/vec_vec_ZZ_p.h>
#include <NTL/mat_ZZ.h>
#include <NTL/matrix.h>
#include <NTL/LLL.h>

// Include Boost Header
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/io.hpp>
#include <boost/progress.hpp>

// Include R Header for graphs
#ifdef WITH_R
#include <RInside.h>
#endif

// Include Random Generator of MRG Matrix
#include "SimpleMRG.h"
#include "Tools.h"


using namespace std;
using namespace NTL;
using namespace LatticeTester;


// projection parameters definition
//----------------------------------------------------------------------------------------

/*
 * if true the output is written in a .txt file
 * if false the ouput is printed in the console
 * via the std::outFile command
 */
bool outFileRequested = false;


/*
 * Type of the basis input.
 * If true, all entry of the basis will be random. In that case,
 * the Dual can't be compute.
 * If false, the basis will be genereated from a MRG with random
 * parameters a = (a1, a2, ..., ak) and a modulus configurable below
 * and according to L'Ecuyer's publications.
 */
bool FullRandomMatrix = false;

/*
 * Select the interval of value for the full random basis.
 * The value will be chosen between minCoeff and maxCoeff.
 * FullRandomMatrix flag must be true.
 */
const int minCoeff = 40;
const int maxCoeff = 1000;

/*
 * All Reducer method, except redDieter, can be used
 * without the computation of the Dual. In that case,
 * we save memory and time on average, and the result is
 * the same. But, for a high period (about 2^40), the
 * Cholesky decomposition computation needs very large number.
 * Thus, the calcul without Dual can drive sometimes to a
 * longer commputing timing.
 * FullRandomMatrix flag must be false.
 */
bool WITH_DUAL = false;

/*
 * Order of the Basis according to L'Écuyer's paper.
 * FullRandomMatrix flag must be false.
 */
const int order = 3;

/*
 * Modulo of the Basis according to L'Écuyer's paper.
 * FullRandomMatrix flag must be false.
 */
const ZZ modulusRNG = power_ZZ(2, 20) - 1;

/*
 * The Dimensional interval to be analysed.
 * Must be int value.
 */
const int MinDimension = 12;
//const int MaxDimension = 12;




#ifdef PRINT_CONSOLE
const int MaxDimension = MinDimension + 1;
#else
const int MaxDimension = 12;
#endif
const int Interval_dim = MaxDimension - MinDimension;



/*
 * Iteration loop over basis of same dimension.
 * Each random basis is computed with a different seed.
 */
const int maxIteration = 30;

/*
 * a/b is the value of the delta in the LLL and BKZ
 * Reduction. NTL offers the possibility to compute
 * a LLL Reduction with the exact delta. We have noticed
 * only minor differences with this option.
 */
const double delta = 0.99999;
const double epsilon = 1.0 - delta;

/*
 * Reduction bound in the RedLLL algorithm.
 */
const int maxcpt = 10000000;

/*
 * Block Size in the BKZ algorithm. See NTL documention
 * for further information.
 */
const long blocksize = 20;

/*
 * Maximum number of Nodes in the Branch-and-Bound.
 */
const int maxNodesBB = 1000000;

/*
 * Selecting method of reducing.
 */
ReduceType Reduce_type[] ={
   Initial,                   // Calibration
   PairRed,                   // Performs Pairwise Reduction
   PairRedRandomized,         // Performs stochastic Pairwise Reduction
   RedLLL,                       // Performs LLL Reduction
   PairRed_LLL,               // Performs Pairwise Reduction
   PairRedRandomized_LLL,     // Perform Pairwise Reduction and then
                              // LLL Reduction
   LLLNTL,                    // Performs LLL Reduction with NTL Library
   PairRed_LLLNTL,            // Perform Pairwise Reduction and then
                              // LLL Reduction with NTL Library
   PairRedRandomized_LLLNTL,  // Perform stocastic Pairwise Reduction and
                              // then LLL Reduction with NTL Library
   BKZNTL,                    // Performs BKZ Reduction with NTL Library
   PairRed_BKZNTL,            // Perform Pairwise Reduction and then
                              // BKZ Reduction with NTL Library
   PairRedRandomized_BKZNTL,  // Perform stocastic Pairwise Reduction and
                              // then BKZ Reduction with NTL Library
   //BB_Only,                   // Performs Branch-and-Bound Reduction
   BB_Classic,                // Perform Pairwise Reduction and then
                              // Branch-and-Bound Reduction
   BB_BKZ                     // Performs BKZ Reduction with NTL Library
                              // and then Branch-and-Bound Reduction
   //RedDieter,                    // Performs Dieter Reduction
                              //WARNING USE DIETER ONLY FOR DIM < 6
   //RedMinkowski                  // Perform Minkowski Reduction with
                              // Branch-and-Bound Algorithm.
};

//----------------------------------------------------------------------------------------



// functions used in main program
//----------------------------------------------------------------------------------------

#ifdef WITH_R
template <typename Type, unsigned long Size, unsigned long Size2>
Rcpp::NumericMatrix toRcppMatrix( const array<array<Type, Size>, Size2> array)
{

   Rcpp::NumericMatrix mat(maxIteration, Interval_dim);
   for (int i = 0; i<Size ; i++) {
      for (int j = 0; j<Size2; j++) {
         conv(mat(i,j), array[j][i]);
      }
   }
   return mat;
}
#endif


bool reduce(
   Reducer & red,
   const ReduceType & name,
   int & seed_dieter,
   const int & blocksize,
   const double & delta,
   const int maxcpt,
   int dimension,
   ZZ det)
{

   bool ok(true);

   switch(name) {
   case PairRed : {
         // Pairwise reduction in primal basis only
         red.preRedDieter(0);
      }
      break;

   case PairRedRandomized : {
         // Randomized pairwise reduction in primal basis only
         red.preRedDieterRandomized(0, seed_dieter);
      }
      break;

   case RedLLL : {
         // LLL Reduction
         red.redLLL(delta, maxcpt, dimension);
      }
      break;

   case PairRed_LLL : {
         // Pairwise reduction (in primal basis only) and then LLL
         red.preRedDieter(0);
      }
      break;

   case PairRedRandomized_LLL : {
         // Randomized pairwise reduction (in primal basis only) and then LLL
         red.preRedDieterRandomized(0, seed_dieter);
      }
      break;

   case LLLNTL : {
         // LLL NTL reduction (floating point version = proxy)
         red.redLLLNTLProxy(delta);
      }
      break;

   case PairRed_LLLNTL : {
         // Pairwise reduction (in primal basis only) and then LLL NTL proxy
         red.preRedDieter(0);
      }
      break;

   case PairRedRandomized_LLLNTL : {
         // Randomized pairwise reduction (in primal basis only) and then LLL NTL proxy
         red.preRedDieterRandomized(0, seed_dieter);
      }
      break;

   case BKZNTL : {
         // BKZ NTL reduction
         red.redBKZ(delta, blocksize);
      }
      break;

   case PairRed_BKZNTL : {
         // Pairwise reduction (in primal basis only) and then BKZ NTL proxy
         red.preRedDieter(0);
      }
      break;

   case PairRedRandomized_BKZNTL : {
         // Randomized pairwise reduction (in primal basis only) and then BKZ NTL proxy
         red.preRedDieterRandomized(0, seed_dieter);
      }
      break;

   case BB_Classic : {
         // Branch and Bound classic
         red.preRedDieter(0);
         red.redLLL(delta, maxcpt, dimension);
      }
      break;

   case BB_BKZ : {
         // Branch and Bound post BKZ
         red.redBKZ(delta, blocksize);
      }
      break;

   case RedDieter : {
         cout << "name : " << name << endl;
         // Dieter Method
         red.shortestVectorDieter(L2NORM);
      }
      break;

   case RedMinkowski : {
         // Minkowski reduction
         ok = red.reductMinkowski(0);
      }
      break;

   default : break;
   }
   return ok;
}


bool reduce2(Reducer & red, const ReduceType & name, int & seed_dieter, const int & blocksize, const double & delta, const int maxcpt, int dimension){
   //outFile << name << endl;

   bool ok(true);

   switch(name) {
   case PairRed_LLL : {
         // Pairwise reduction (in primal basis only) and then LLL
         red.redLLL(delta, maxcpt, dimension);
      }
      break;

   case PairRedRandomized_LLL : {
         // Randomized pairwise reduction (in primal basis only) and then LLL
         red.redLLL(delta, maxcpt, dimension);
      }
      break;

   case PairRed_LLLNTL : {
         // Pairwise reduction (in primal basis only) and then LLL NTL proxy
         red.redLLLNTLProxy(delta);
      }
      break;

   case PairRedRandomized_LLLNTL : {
         // Randomized pairwise reduction (in primal basis only) and then LLL NTL proxy
         red.redLLLNTLProxy(delta);
      }
      break;

   case PairRed_BKZNTL : {
         // Pairwise reduction (in primal basis only) and then BKZ NTL proxy
         red.redBKZ(delta, blocksize);
      }
      break;

   case PairRedRandomized_BKZNTL : {
         // Randomized pairwise reduction (in primal basis only) and then BKZ NTL proxy
         red.redBKZ(delta, blocksize);
      }
      break;

   case BB_Classic : {
         // Branch and Bound classic
         ok = red.shortestVector(L2NORM);
      }
      break;

   case BB_BKZ : {
         // Branch and Bound post BKZ
         ok = red.shortestVector(L2NORM);
      }
      break;
   default : break;
   }
   return ok;
}



//****************************************************************************************************
//****************************************************************************************************

int main (int argc, char *argv[])
{
   ofstream realOutFile;
   string fileName;
   if (outFileRequested) {
      cout << "Enter file name (without .txt extension): ";
      cin >> fileName;
   }
   ostream & outFile = outFileRequested ? realOutFile.open(fileName+".txt", std::ios::out), realOutFile : std::cout;

   //ofstream realOutFile;
   //ostream & outFile = outFileRequested ? realOutFile.open("nik.txt", std::ios::out), realOutFile : std::cout;

   // printing total running time
   clock_t begin = clock();

   outFile << endl;

   // Stock Results
   map<ReduceType, array< array<NScal, maxIteration>, Interval_dim > > length_results;
   map<ReduceType, array< array<double, maxIteration>, Interval_dim > > timing_results;
   map<ReduceType, array< array<double, maxIteration>, Interval_dim > > timing_results2; // 2nd Stage
   map<ReduceType, int> nb_diff;

   // old declaration using C-style arrays not accepted by some compilators
   //map<string, map<int, NScal[maxIteration]> > length_results;
   //map<string, map<int, double[maxIteration]> > timing_results;

   // to display progress bar
   boost::progress_display show_progress(maxIteration*Interval_dim);

   // arrays to store values
   int id_dimension = 0;
   bool all_BB_over = true;
   int nb_error = 0;

   for (int dimension = MinDimension; dimension < MaxDimension; dimension++){

      id_dimension = dimension - MinDimension;

      for (int iteration = 0; iteration < maxIteration; iteration++){
         do{
            if(!all_BB_over){
               outFile << "/";
               nb_error++; // Pour la boucle
            }
            all_BB_over = true;
            ZZ seedZZ = conv<ZZ>((iteration+1) * (iteration+1) * 123456789 * dimension * (nb_error+1));
            int seed = (iteration+1) * (iteration+1) * 123456789 * dimension * (nb_error+1);
            int seed_dieter = (iteration+1) * dimension * 12342 * (nb_error+1) ;
            //int seed = (int) (iteration+1) * 12345 * time(NULL);

            // We create copies of the same basis
            BMat basis_PairRed (dimension, dimension);
            ZZ det;
            BMat V;
            BMat W;
            if(FullRandomMatrix){
               V = RandomMatrix(dimension, det, minCoeff, maxCoeff, seed);
               WITH_DUAL = false;
            }
            else{
               V = CreateRNGBasis (modulusRNG, order, dimension, seedZZ);
               if(WITH_DUAL){
                  W = Dualize (V, modulusRNG, order);
               }
            }

            map < ReduceType, BMat* > basis;
            map < ReduceType, BMat* > dualbasis;
            map < ReduceType, IntLatticeBasis* > lattices;
            map < ReduceType, Reducer* > reducers;
            map < ReduceType, clock_t > timing;

            for(const ReduceType name : Reduce_type){
               basis[name] = new BMat(V);
               if(WITH_DUAL){
                  dualbasis[name] = new BMat(W);
                  lattices[name] = new IntLatticeBasis(*basis[name], *dualbasis[name], modulusRNG, dimension);
               }
               else{
                  lattices[name] = new IntLatticeBasis(*basis[name], dimension);
               }
               // IF WE WANT FULL RANDOM MATRIX
               //basis[name] = new BMat(basis_PairRed);

               reducers[name] = new Reducer(*lattices[name]);
            }
            clock_t begin = clock();
            clock_t end = clock();

            bool ok = true;
            for(const ReduceType &name : Reduce_type){
               begin = clock();
               ok = reduce(*reducers[name], name, seed_dieter, blocksize, delta, maxcpt, dimension, det);
               end = clock();
               all_BB_over = all_BB_over && ok;
               timing_results[name][id_dimension][iteration] = double (end - begin) / CLOCKS_PER_SEC;
               lattices[name]->setNegativeNorm();
               lattices[name]->updateVecNorm();
               lattices[name]->sort(0);
               //outFile << "Norm of " << name << " : " << lattices[name]->getVecNorm(0) << endl;

            }

            for(const ReduceType &name : Reduce_type){
               //outFile << "Norm of " << name << " : " << lattices[name]->getVecNorm(0) << endl;
               begin = clock();
               ok = reduce2(*reducers[name], name, seed_dieter, blocksize, delta, maxcpt, dimension);
               end = clock();
               all_BB_over = all_BB_over && ok;
               timing_results2[name][id_dimension][iteration] = double (end - begin) / CLOCKS_PER_SEC;
               lattices[name]->setNegativeNorm();
               lattices[name]->updateVecNorm();
               lattices[name]->sort(0);
               //outFile << "Norm of " << name << " : " << lattices[name]->getVecNorm(0) << endl;
            }



            for(const ReduceType &name : Reduce_type){
               if((lattices[name]->getVecNorm(0) - lattices[BB_Classic]->getVecNorm(0)) > 0.1)
                  nb_diff[name]++;
               length_results[name][id_dimension][iteration] = lattices[name]->getVecNorm(0);
            }


            for(const ReduceType &name : Reduce_type){
               basis[name]->BMat::clear();
               if(WITH_DUAL)
                  dualbasis[name]->BMat::clear();
               //-dualbasis[name]->kill();
               delete lattices[name];
               delete reducers[name];
            }

         } while(!all_BB_over);
         ++show_progress;
      }

   } // end iteration loop over matrices of same dimension


#ifdef PRINT_CONSOLE

   //------------------------------------------------------------------------------------
   // Results printing in console
   //------------------------------------------------------------------------------------

   // print parameters used
   outFile << "\n" << endl;
   outFile << "epsilon = " << epsilon << endl;
   outFile << "dimension = " << MinDimension << endl;
   outFile << "nombre de matrices testées = " << maxIteration << endl;
   outFile << "ordre de la matrice : " << order << endl;
   outFile << "Nombre de Branch and Bound non terminés : " << nb_error << endl;
   if(WITH_DUAL)
      outFile << "Dual utilisé" << endl;
   else
      outFile << "Dual non utilisé" << endl;

   // print statistics
   outFile << "\n     ---------------- TIMING AVG ----------------\n" << endl;

   outFile << "             PairRed = " << mean(timing_results[PairRed][0]) << " (+/- " << variance(timing_results[PairRed][0]) << ")" << endl;
   outFile << "       PairRedRandom = " << mean(timing_results[PairRedRandomized][0]) << " (+/- " << variance(timing_results[PairRedRandomized][0]) << ")" << endl;
   outFile << endl;

   outFile << "                 LLL = " << mean(timing_results[RedLLL][0]) << " (+/- " << variance(timing_results[RedLLL][0]) << ")" << endl;
   outFile << "         PairRed+LLL = " << mean(timing_results[PairRed_LLL][0]) + mean(timing_results2[PairRed_LLL][0]);
   outFile << " (" << mean(timing_results2[PairRed_LLL][0]) << ") (+/- " << variance(timing_results2[PairRed_LLL][0]) << ")" << endl;
   outFile << "   PairRedRandom+LLL = " << mean(timing_results[PairRedRandomized_LLL][0]) + mean(timing_results2[PairRedRandomized_LLL][0]);
   outFile << " (" << mean(timing_results2[PairRedRandomized_LLL][0]) << ") (+/- " << variance(timing_results2[PairRedRandomized_LLL][0]) << ")" << endl;
   outFile << endl;

   outFile << "              LLLNTL = " << mean(timing_results[LLLNTL][0]) << " (+/- " << variance(timing_results[LLLNTL][0]) << ")" << endl;
   outFile << "      PairRed+LLLNTL = " << mean(timing_results[PairRed_LLLNTL][0]) + mean(timing_results2[PairRed_LLLNTL][0]);
   outFile << " (" << mean(timing_results2[PairRed_LLLNTL][0]) << ") (+/- " << variance(timing_results[PairRed_LLLNTL][0]) << ")" << endl;
   outFile << "PairRedRandom+LLLNTL = " << mean(timing_results[PairRedRandomized_LLLNTL][0]) + mean(timing_results2[PairRedRandomized_LLLNTL][0]);
   outFile << " (" << mean(timing_results2[PairRedRandomized_LLLNTL][0]) << ") (+/- " << variance(timing_results2[PairRedRandomized_LLLNTL][0]) << ")" << endl;
   outFile << endl;

   //outFile << "        LLLNTL_Exact = " << mean(timing_LLL_NTL_Exact) << endl;
   outFile << endl;

   outFile << "              BKZNTL = " << mean(timing_results[BKZNTL][0]) << ") (+/- " << variance(timing_results[BKZNTL][0]) << ")" << endl;
   outFile << "      PairRed+BKZNTL = " << mean(timing_results[PairRed_BKZNTL][0]) + mean(timing_results2[PairRed_BKZNTL][0]);
   outFile << " (" << mean(timing_results2[PairRed_BKZNTL][0]) << ") (+/- " << variance(timing_results2[PairRed_BKZNTL][0]) << ")" << endl;
   outFile << "PairRedRandom+BKZNTL = " << mean(timing_results[PairRedRandomized_BKZNTL][0]) + mean(timing_results2[PairRedRandomized_BKZNTL][0]);
   outFile << " (" << mean(timing_results2[PairRedRandomized_BKZNTL][0]) << ") (+/- " << variance(timing_results2[PairRedRandomized_BKZNTL][0]) << ")" << endl;
   outFile << endl;

   outFile << "          BB Classic = " << mean(timing_results[BB_Classic][0]) + mean(timing_results2[BB_Classic][0]);
   outFile << " (" << mean(timing_results2[BB_Classic][0]) << ") (+/- " << variance(timing_results2[BB_Classic][0]) << ")" ")" << endl,
   outFile << "              BB BKZ = " << mean(timing_results[BB_BKZ][0]) + mean(timing_results2[BB_BKZ][0]);
   outFile << " (" << mean(timing_results2[BB_BKZ][0]) << ") (+/- " << variance(timing_results2[BB_BKZ][0]) << ")";
   outFile << endl;

   //outFile << "    Dieter Reduction = " << mean(timing_results[RedDieter][0]);
   //if (!WITH_DUAL)
   //   outFile << " DIETER NON EFFECTUER CAR DUAL NECESSAIRE" << endl;
   //else
   //   outFile << endl;
   //outFile << " Minkowski Reduction = " << mean(timing_results[RedMinkowski][0]) << endl;

   outFile << "\n     --------------------------------------------" << endl;



   outFile << "\n     ---------------- LENGTH AVG ----------------\n" << endl;

   outFile << "             Initial = " << conv<ZZ>(mean(length_results[Initial][0])) << "   Error Rate : " << (double) nb_diff[Initial]/maxIteration << endl;

   outFile << "             PairRed = " << conv<ZZ>(mean(length_results[PairRed][0])) << "   Error Rate : " << (double) nb_diff[PairRed]/maxIteration << endl;
   outFile << "       PairRedRandom = " << conv<ZZ>(mean(length_results[PairRedRandomized][0])) << "   Error Rate : " << (double) nb_diff[PairRedRandomized]/maxIteration << endl;
   outFile << endl;

   outFile << "                 LLL = " << conv<ZZ>(mean(length_results[RedLLL][0])) << "   Error Rate : " << (double) nb_diff[RedLLL]/maxIteration << endl;
   outFile << "         PairRed+LLL = " << conv<ZZ>(mean(length_results[PairRed_LLL][0])) << "   Error Rate : " << (double) nb_diff[PairRed_LLL]/maxIteration << endl;
   outFile << "   PairRedRandom+LLL = " << conv<ZZ>(mean(length_results[PairRedRandomized_LLL][0])) << "   Error Rate : " << (double) nb_diff[PairRedRandomized_LLL]/maxIteration << endl;
   outFile << endl;

   outFile << "              LLLNTL = " << conv<ZZ>(mean(length_results[LLLNTL][0])) << "   Error Rate : " << (double) nb_diff[LLLNTL]/maxIteration << endl;
   outFile << "      PairRed+LLLNTL = " << conv<ZZ>(mean(length_results[PairRed_LLLNTL][0])) << "   Error Rate : " << (double) nb_diff[PairRed_LLLNTL]/maxIteration << endl;
   outFile << "PairRedRandom+LLLNTL = " << conv<ZZ>(mean(length_results[PairRedRandomized_LLLNTL][0])) << "   Error Rate : " << (double) nb_diff[PairRedRandomized_LLLNTL]/maxIteration << endl;
   outFile << endl;
   outFile << endl;

   outFile << "              BKZNTL = " << conv<ZZ>(mean(length_results[BKZNTL][0])) << "   Error Rate : " << (double) nb_diff[BKZNTL]/maxIteration << endl;
   outFile << "      PairRed+BKZNTL = " << conv<ZZ>(mean(length_results[PairRed_BKZNTL][0])) << "   Error Rate : " << (double) nb_diff[PairRed_BKZNTL]/maxIteration << endl;
   outFile << "PairRedRandom+BKZNTL = " << conv<ZZ>(mean(length_results[PairRedRandomized_BKZNTL][0])) << "   Error Rate : " << (double) nb_diff[PairRedRandomized_BKZNTL]/maxIteration << endl;
   outFile << endl;

   //outFile << "             BB Only = " << conv<ZZ>(mean(length_results[PairRed_BKZNTL][0])) << "   Error Rate : " << (double) nb_diff[am]/maxIteration << endl;
   outFile << "          BB Classic = " << conv<ZZ>(mean(length_results[BB_Classic][0])) << "   Error Rate : " << (double) nb_diff[BB_Classic]/maxIteration << endl,
   outFile << "              BB BKZ = " << conv<ZZ>(mean(length_results[BB_BKZ][0])) << "   Error Rate : " << (double) nb_diff[BB_BKZ]/maxIteration << endl;
   outFile << endl;

   //outFile << "    Dieter Reduction = " << conv<ZZ>(mean(length_results[RedDieter][0])) << "   Error Rate : " << (double) nb_diff[RedDieter]/maxIteration << endl,
   //outFile << " Minkowski Reduction = " << conv<ZZ>(mean(length_results[RedMinkowski][0])) << "   Error Rate : " << (double) nb_diff[RedMinkowski]/maxIteration << endl;

   outFile << "\n     --------------------------------------------\n" << endl;


#endif



#ifdef WITH_R
   /*----------------------------------------------*/
   // UTILISATION DE R
   /*----------------------------------------------*/


   RInside R(argc, argv);              // create an embedded R instance

   R["MinDimension"] = MinDimension;
   R["Maxdimension"] = MaxDimension;
   R["dimension"] = Interval_dim;
   //R["timing_Initial"] = toRcppMatrix(timing_Initial, maxIteration);
   for(const ReduceType &name : Reduce_type){
      R[name] = toRcppMatrix(timing_results[name]);
   }

    // by running parseEval, we get the last assignment back, here the filename

   std::string outPath = "~/Desktop";
   std::string outFile = "myPlot.png";
   R["outPath"] = outPath;
   R["outFile"] = outFile;

   // alternatively, by forcing a display we can plot to screen
   string library = "library(ggplot2); ";
   string build_data_frame = "df <- data.frame(indice = seq(1:dimension)";
   for(const string &name : names){
      build_data_frame += ", col_" + name + " =colMeans(" + name + ")";
   }
   build_data_frame += ");";


   string build_plot = "myPlot <- ggplot() + ";
   for(const string &name : names){
      build_plot += "geom_line(data=df, aes(x=indice, y=col_" + name + ", color ='col_" + name + "')) + ";
   }
   build_plot += "labs(color='Legend text'); ";

   string print_plot =
    "print(myPlot); "
     "ggsave(filename=outFile, path=outPath, plot=myPlot); ";
   // parseEvalQ evluates without assignment
   R.parseEvalQ(library);
   R.parseEvalQ(build_data_frame);
   R.parseEvalQ(build_plot);
   R.parseEvalQ(print_plot);

#endif

   // printing total running time
   clock_t end = clock();
   outFile << "\nTotal running time = " << (double) (end - begin) / CLOCKS_PER_SEC << endl;


   if (outFileRequested)
      realOutFile.close();

#endif

   return 0;
}

