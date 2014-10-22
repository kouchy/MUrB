/*
 * Do not remove.
 * Optimization training courses 2014 (CINES)
 * Adrien Cassagne, adrien.cassagne@cines.fr
 * This file is under CC BY-NC-ND license (http://creativecommons.org/licenses/by-nc-nd/4.0/legalcode)
 */

#ifdef NBODY_DOUBLE
using floatType = double;
#else
using floatType = float;
#endif

#include <map>
#include <cmath>
#include <string>
#include <vector>
#include <cassert>
#include <iostream>
using namespace std;

#include "ogl/SpheresVisu.h"
#include "ogl/SpheresVisuNo.h"
#ifdef VISU
#include "ogl/OGLSpheresVisuGS.h"
#include "ogl/OGLSpheresVisuInst.h"
#endif

#include "utils/Perf.h"
#include "utils/ArgumentsReader.h"

#include "core/Bodies.h"
#include "core/SimulationNBody.h"
#include "core/v1/local/SimulationNBodyV1.h"
#include "core/v1/local/SimulationNBodyV1CB.h"
#include "core/v1/local/SimulationNBodyV1Vectors.h"
#include "core/v1/local/SimulationNBodyV1Intrinsics.h"
#include "core/v2/local/SimulationNBodyV2.h"
#include "core/v2/local/SimulationNBodyV2CB.h"
#include "core/v2/local/SimulationNBodyV2Vectors.h"
#include "core/v2/local/SimulationNBodyV2Intrinsics.h"
#include "core/v2/local/SimulationNBodyV2FineTuned.h"

#include "core/collisionv1/local/SimulationNBodyCollisionV1.h"

#ifdef _OPENMP
#include <omp.h>
#else
#ifndef NO_OMP
#define NO_OMP
inline void omp_set_num_threads(int) {           }
inline int  omp_get_num_threads(   ) { return 1; }
inline int  omp_get_max_threads(   ) { return 1; }
inline int  omp_get_thread_num (   ) { return 0; }
#endif
#endif

#ifdef USE_MPI
#include <mpi.h>
#include "core/v1/mpi/SimulationNBodyMPIV1.h"
#include "core/v1/mpi/SimulationNBodyMPIV1Intrinsics.h"
#else
#ifndef NO_MPI
#define NO_MPI
namespace MPI
{
	inline void Init()     {}
	inline void Finalize() {}

	class Comm {
	public:
		                   Comm     (       ) {            };
		virtual            ~Comm    (       ) {            };
		static inline int  Get_rank (       ) { return 0;  };
		static inline int  Get_size (       ) { return 1;  };
		static inline void Barrier  (       ) {            };
		static inline void Abort    (int val) { exit(val); };
	};

	Comm COMM_WORLD;
}
#endif
#endif

/* global variables */
string        RootInputFileName;
string        RootOutputFileName;
unsigned long NBodies;
unsigned long NIterations;
unsigned int  ImplId     = 10;
bool          Verbose    = false;
bool          GSEnable   = false;
bool          VisuEnable = true;
bool          DtVariable = false;
floatType     Dt         = 3600; //in sec, 3600 sec = 1 hour
unsigned int  WinWidth   = 800;
unsigned int  WinHeight  = 600;

/*
 * read args from command line and set global variables
 * usage: ./nbody -n nBodies  -i nIterations [-v] [-w] ...
 * usage: ./nbody -f fileName -i nIterations [-v] [-w] ...
 * */
void argsReader(int argc, char** argv)
{
	map<string, string> reqArgs1, reqArgs2, faculArgs, docArgs;
	ArgumentsReader argsReader(argc, argv);

	reqArgs1 ["n"]     = "nBodies";
	docArgs  ["n"]     = "the number of bodies randomly generated.";
	reqArgs1 ["i"]     = "nIterations";
	docArgs  ["i"]     = "the number of iterations to compute.";

	reqArgs2 ["f"]     = "rootInputFileName";
	docArgs  ["f"]     = "the root file name of the body file(s) to read, do not use with -n "
	                     "(you can put 'data/in/p1/8bodies').";
	reqArgs2 ["i"]     = "nIterations";

	faculArgs["v"]     = "";
	docArgs  ["v"]     = "enable verbose mode.";
	faculArgs["w"]     = "rootOutputFileName";
	docArgs  ["w"]     = "the root file name of the body file(s) to write (you can put 'data/out/bodies').";
	faculArgs["h"]     = "";
	docArgs  ["h"]     = "display this help.";
	faculArgs["-help"] = "";
	docArgs  ["-help"] = "display this help.";
	faculArgs["-dt"]   = "timeStep";
	docArgs  ["-dt"]   = "select a fixed time step in second (default is " + to_string(Dt) + " sec).";
	faculArgs["-gs"]   = "";
	docArgs  ["-gs"]   = "enable geometry shader for visu, "
	                     "this is faster than the standard way but not all GPUs can support it.";
	faculArgs["-ww"]   = "winWidth";
	docArgs  ["-ww"]   = "the width of the window in pixel (default is " + to_string(WinWidth) + ").";
	faculArgs["-wh"]   = "winHeight";
	docArgs  ["-wh"]   = "the height of the window in pixel (default is " + to_string(WinHeight) + ").";
	faculArgs["-nv"]   = "";
	docArgs  ["-nv"]   = "no visualization (disable visu).";
	faculArgs["-vdt"]   = "";
	docArgs  ["-vdt"]   = "enable variable time step.";
	faculArgs["-im"]   = "ImplId";
	docArgs  ["-im"]   = "code implementation id (value should be 10, 11, 12, 13, 20, 21, 22, 23, 100 or 103).";

	if(argsReader.parseArguments(reqArgs1, faculArgs))
	{
		NBodies           = stoi(argsReader.getArgument("n")) / MPI::COMM_WORLD.Get_size();
		NIterations       = stoi(argsReader.getArgument("i"));
		RootInputFileName = "";
	}
	else if(argsReader.parseArguments(reqArgs2, faculArgs))
	{
		RootInputFileName = argsReader.getArgument("f");
		NIterations       = stoi(argsReader.getArgument("i"));
	}
	else
	{
		if(argsReader.parseDocArgs(docArgs))
			argsReader.printUsage();
		else
			cout << "A problem was encountered when parsing arguments documentation... exiting." << endl;
		exit(-1);
	}

	if(argsReader.existArgument("h") || argsReader.existArgument("-help"))
	{
		if(argsReader.parseDocArgs(docArgs))
			argsReader.printUsage();
		else
			cout << "A problem was encountered when parsing arguments documentation... exiting." << endl;
		exit(-1);
	}

	if(argsReader.existArgument("v"))
		Verbose = true;
	if(argsReader.existArgument("w"))
		RootOutputFileName = argsReader.getArgument("w");
	if(argsReader.existArgument("-dt"))
		Dt = stof(argsReader.getArgument("-dt"));
	if(argsReader.existArgument("-gs"))
		GSEnable = true;
	if(argsReader.existArgument("-ww"))
		WinWidth = stoi(argsReader.getArgument("-ww"));
	if(argsReader.existArgument("-wh"))
		WinHeight = stoi(argsReader.getArgument("-wh"));
	if(argsReader.existArgument("-nv"))
		VisuEnable = false;
	if(argsReader.existArgument("-vdt"))
		DtVariable = true;
	if(argsReader.existArgument("-im"))
		ImplId = stoi(argsReader.getArgument("-im"));
}

template <typename T>
string strDate(T timestamp)
{
	unsigned int days;
	unsigned int hours;
	unsigned int minutes;
	T rest;

	days = timestamp / (24 * 60 * 60);
	rest = timestamp - (days * 24 * 60 * 60);

	hours = rest / (60 * 60);
	rest = rest - (hours * 60 * 60);

	minutes = rest / 60;
	rest = rest - (minutes * 60);

	return to_string(days)    + "d " +
	       to_string(hours)   + "h " +
	       to_string(minutes) + "m " +
	       to_string(rest)    + "s";
}

template <typename T>
SimulationNBody<T>* selectImplementationAndAllocateSimulation()
{
	SimulationNBody<floatType> *simu = nullptr;

	string inputFileName = RootInputFileName + ".p" + to_string(MPI::COMM_WORLD.Get_rank()) + ".dat";

	switch(ImplId)
	{
		case 10:
			if(!MPI::COMM_WORLD.Get_rank())
				cout << "Selected implementation: V1 - O(n²)" << endl << endl;
			if(RootInputFileName.empty())
				simu = new SimulationNBodyV1<T>(NBodies);
			else
				simu = new SimulationNBodyV1<T>(inputFileName);
			break;
		case 11:
			if(!MPI::COMM_WORLD.Get_rank())
				cout << "Selected implementation: V1 + cache blocking - O(n²)" << endl << endl;
			if(RootInputFileName.empty())
				simu = new SimulationNBodyV1CB<T>(NBodies);
			else
				simu = new SimulationNBodyV1CB<T>(inputFileName);
			break;
		case 12:
			if(!MPI::COMM_WORLD.Get_rank())
				cout << "Selected implementation: V1 + vectors - O(n²)" << endl << endl;
			if(RootInputFileName.empty())
				simu = new SimulationNBodyV1Vectors<T>(NBodies);
			else
				simu = new SimulationNBodyV1Vectors<T>(inputFileName);
			break;
		case 13:
			if(!MPI::COMM_WORLD.Get_rank())
				cout << "Selected implementation: V1 + intrinsics - O(n²)" << endl << endl;
			if(RootInputFileName.empty())
				simu = new SimulationNBodyV1Intrinsics<T>(NBodies);
			else
				simu = new SimulationNBodyV1Intrinsics<T>(inputFileName);
			break;
		case 14:
			if(!MPI::COMM_WORLD.Get_rank())
				cout << "Selected implementation: V1 + collisions - O(n²)" << endl << endl;
			if(RootInputFileName.empty())
				simu = new SimulationNBodyCollisionV1<T>(NBodies);
			else
				simu = new SimulationNBodyCollisionV1<T>(inputFileName);
			break;
		case 20:
			if(!MPI::COMM_WORLD.Get_rank())
				cout << "Selected implementation: V2 - O(n²/2)" << endl << endl;
			if(RootInputFileName.empty())
				simu = new SimulationNBodyV2<T>(NBodies);
			else
				simu = new SimulationNBodyV2<T>(inputFileName);
			break;
		case 21:
			if(!MPI::COMM_WORLD.Get_rank())
				cout << "Selected implementation: V2 + cache blocking - O(n²/2)" << endl << endl;
			if(RootInputFileName.empty())
				simu = new SimulationNBodyV2CB<T>(NBodies);
			else
				simu = new SimulationNBodyV2CB<T>(inputFileName);
			break;
		case 22:
			if(!MPI::COMM_WORLD.Get_rank())
				cout << "Selected implementation: V2 + vectors - O(n²/2)" << endl << endl;
			if(RootInputFileName.empty())
				simu = new SimulationNBodyV2Vectors<T>(NBodies);
			else
				simu = new SimulationNBodyV2Vectors<T>(inputFileName);
			break;
		case 23:
			cout << "Selected implementation: V2 + intrinsics - O(n²/2)" << endl << endl;
			if(RootInputFileName.empty())
				simu = new SimulationNBodyV2Intrinsics<T>(NBodies);
			else
				simu = new SimulationNBodyV2Intrinsics<T>(inputFileName);
			break;
		case 24:
			cout << "Selected implementation: V2 + intrinsics fine tuned - O(n²/2)" << endl << endl;
			if(RootInputFileName.empty())
				simu = new SimulationNBodyV2FineTuned<T>(NBodies);
			else
				simu = new SimulationNBodyV2FineTuned<T>(inputFileName);
			break;
#ifndef NO_MPI
		case 100:
			if(!MPI::COMM_WORLD.Get_rank())
				cout << "Selected implementation: V1 MPI - O(n²)" << endl << endl;
			if(RootInputFileName.empty())
				simu = new SimulationNBodyMPIV1<T>(NBodies);
			else
				simu = new SimulationNBodyMPIV1<T>(inputFileName);
			break;
		case 103:
			if(!MPI::COMM_WORLD.Get_rank())
				cout << "Selected implementation: V1 MPI + intrinsics - O(n²)" << endl << endl;
			if(RootInputFileName.empty())
				simu = new SimulationNBodyMPIV1Intrinsics<T>(NBodies);
			else
				simu = new SimulationNBodyMPIV1Intrinsics<T>(inputFileName);
			break;
#endif
		default:
			if(!MPI::COMM_WORLD.Get_rank())
				cout << "This code implementation does not exist... Exiting." << endl;
			MPI::Finalize();
			exit(-1);
			break;
	}

	if(ImplId < 100 && MPI::COMM_WORLD.Get_size() > 1)
	{
		if(!MPI::COMM_WORLD.Get_rank())
			cout << "Implementation n°" << ImplId << " is not an MPI implementation... Exiting." << endl;
		MPI::Finalize();
		exit(-1);
	}

	return simu;
}

template <typename T>
SpheresVisu* selectImplementationAndAllocateVisu(SimulationNBody<T> *simu)
{
	SpheresVisu* visu;

#ifdef VISU
	// only the MPI proc 0 can display the bodies
	if(MPI::COMM_WORLD.Get_rank())
		VisuEnable = false;

	if(VisuEnable)
	{
		const T *positionsX = simu->getBodies().getPositionsX();
		const T *positionsY = simu->getBodies().getPositionsY();
		const T *positionsZ = simu->getBodies().getPositionsZ();
		const T *radiuses   = simu->getBodies().getRadiuses();

		if(GSEnable) // geometry shader = better performances on dedicated GPUs
			visu = new OGLSpheresVisuGS<T>("n-body (geometry shader)", WinWidth, WinHeight,
			                               positionsX, positionsY, positionsZ,
			                               radiuses,
			                               NBodies);
		else
			visu = new OGLSpheresVisuInst<T>("n-body (instancing)", WinWidth, WinHeight,
			                                 positionsX, positionsY, positionsZ,
			                                 radiuses,
			                                 NBodies);
		cout << endl;
	}
	else
		visu = new SpheresVisuNo<T>();
#else
	VisuEnable = false;
	visu = new SpheresVisuNo<T>();
#endif

	return visu;
}

template <typename T>
void writeBodies(SimulationNBody<T> *simu, const unsigned long &iIte)
{
	string extension = RootOutputFileName.substr(RootOutputFileName.find_last_of(".") + 1);
	string tmpFileName;
	string outputFileName;

	// each process MPI writes its bodies in a common file
	// TODO: bad perfs
	if(extension == "dat")
	{
		string realRootOutputFileName = RootOutputFileName.substr(0, RootOutputFileName.find_last_of("."));
		tmpFileName = realRootOutputFileName + ".i" + to_string(iIte);
		outputFileName = tmpFileName + ".p0.dat";

		unsigned long MPINBodies = 0;
		if(!MPI::COMM_WORLD.Get_rank())
			MPINBodies = simu->getBodies().getN() * MPI::COMM_WORLD.Get_size();

		for(unsigned long iRank = 0; iRank < MPI::COMM_WORLD.Get_size(); iRank++)
		{
			if(iRank == MPI::COMM_WORLD.Get_rank())
				if(!simu->getBodies().writeIntoFileMPI(outputFileName, MPINBodies))
					MPI::COMM_WORLD.Abort(-1);

			MPI::COMM_WORLD.Barrier();
		}
	}
	else // each process MPI writes its bodies in separate files
	{
		tmpFileName = RootOutputFileName + ".i" + to_string(iIte);
		outputFileName = tmpFileName + ".p" + to_string(MPI::COMM_WORLD.Get_rank()) + ".dat";
		simu->getBodies().writeIntoFile(outputFileName);
	}

	if(Verbose && !MPI::COMM_WORLD.Get_rank())
	{
		tmpFileName = tmpFileName + ".p*.dat";
		cout << "   Writing iteration n°" << iIte << " into \"" << tmpFileName << "\" file(s)." << endl;
	}
}

int main(int argc, char** argv)
{
	MPI::Init();

	// read arguments from the command line
	// usage: ./nbody -f fileName -i nIterations [-v] [-w] ...
	// usage: ./nbody -n nBodies  -i nIterations [-v] [-w] ...
	argsReader(argc, argv);

	// create the n-body simulation
	SimulationNBody<floatType> *simu = selectImplementationAndAllocateSimulation<floatType>();
	const unsigned long n = simu->getBodies().getN();
	NBodies = n;

	// get MB used for this simulation
	float Mbytes = simu->getAllocatedBytes() / 1024.f / 1024.f;

	// display simulation configuration
	if(!MPI::COMM_WORLD.Get_rank())
	{
		cout << "n-body simulation configuration:" << endl;
		cout << "--------------------------------" << endl;
		if(!RootInputFileName.empty())
			cout << "  -> input file name(s)    : " << RootInputFileName << ".p*.dat" << endl;
		else
			cout << "  -> random mode           : enable" << endl;
		if(!RootOutputFileName.empty())
			cout << "  -> output file name(s)   : " << RootOutputFileName << ".i*.p*.dat" << endl;
		cout << "  -> total nb. of bodies   : " << NBodies * MPI::COMM_WORLD.Get_size() << endl;
		cout << "  -> nb. of bodies per proc: " << NBodies << endl;
		cout << "  -> nb. of iterations     : " << NIterations << endl;
		cout << "  -> verbose mode          : " << ((Verbose) ? "enable" : "disable") << endl;
		cout << "  -> precision             : " << ((sizeof(floatType) == 4) ? "simple" : "double") << endl;
		cout << "  -> mem. allocated        : " << Mbytes << " MB" << endl;
		cout << "  -> geometry shader       : " << ((GSEnable) ? "enable" : "disable") << endl;
		cout << "  -> time step             : " << ((DtVariable) ? "variable" : to_string(Dt) + " sec") << endl;
		cout << "  -> nb. of MPI procs      : " << MPI::COMM_WORLD.Get_size() << endl;
		cout << "  -> nb. of threads        : " << omp_get_max_threads() << endl << endl;
	}

	// initialize visualization of bodies (with spheres in space)
	SpheresVisu *visu = selectImplementationAndAllocateVisu<floatType>(simu);

	if(!MPI::COMM_WORLD.Get_rank())
		cout << "Simulation started..." << endl;

	// write initial bodies into file
	if(!RootOutputFileName.empty())
		writeBodies<floatType>(simu, 0);

	// time step selection
	if(!DtVariable)
		simu->setDtConstant(Dt);

	// loop over the iterations
	Perf perfIte, perfTotal;
	floatType physicTime = 0.0;
	unsigned long iIte;
	for(iIte = 1; iIte <= NIterations && !visu->windowShouldClose(); iIte++)
	{
		// refresh the display in OpenGL window
		visu->refreshDisplay();

		// simulation computations
		perfIte.start();
		simu->computeOneIteration();
		perfIte.stop();
		perfTotal += perfIte;

		// compute the elapsed physic time
		physicTime += simu->getDt();

		// display the status of this iteration
		if(Verbose && !MPI::COMM_WORLD.Get_rank())
			cout << "Processing iteration n°" << iIte << " took " << perfIte.getElapsedTime() << " ms "
			     << "(" << perfIte.getGflops(simu->getFlopsPerIte()) << " Gflop/s), "
			     << "physic time: " << strDate(physicTime) << endl;

		// write iteration results into file
		if(!RootOutputFileName.empty())
			writeBodies<floatType>(simu, iIte);
	}
	if(!MPI::COMM_WORLD.Get_rank())
		cout << "Simulation ended." << endl << endl;

	if(!MPI::COMM_WORLD.Get_rank())
		cout << "Entire simulation took " << perfTotal.getElapsedTime() << " ms "
		     << "(" << perfTotal.getGflops(simu->getFlopsPerIte() * (iIte -1)) << " Gflop/s)" << endl;

	// free resources
	delete visu;
	delete simu;

	if(NIterations > (iIte +1))
		MPI::COMM_WORLD.Abort(0);

	MPI::Finalize();

	return EXIT_SUCCESS;
}
