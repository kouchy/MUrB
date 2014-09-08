/*
 * Do not remove.
 * MPI/OpenMP training courses
 * Adrien Cassagne, ASA - CINES, adrien.cassagne@cines.fr
 * This file is under CC BY-NC-ND license (http://creativecommons.org/licenses/by-nc-nd/4.0/legalcode)
 */

#ifdef NBODY_DOUBLE
#define TYPE double
#else
#define TYPE float
#endif

#include <map>
#include <cmath>
#include <string>
#include <vector>
#include <cassert>
#include <iostream>
using namespace std;

#include "ogl/OGLSpheresVisuInst.h"
#include "ogl/OGLSpheresVisuGS.h"
#include "ogl/OGLSpheresVisuNo.h"

#include "utils/Perf.h"
#include "utils/ArgumentsReader.h"

#include "Space.h"

string        InputFileName;
string        OutputBaseName;
unsigned long NBodies;
unsigned long NIterations;
bool          Verbose    = false;
bool          GSEnable   = false;
bool          VisuEnable = true;
TYPE          Dt         = 3600; //in sec, 3600 sec = 1 hour
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
	docArgs  ["n"]     = "approximate number of bodies randomly generated.";
	reqArgs1 ["i"]     = "nIterations";
	docArgs  ["i"]     = "number of iterations to compute.";

	reqArgs2 ["f"]     = "inputFileName";
	docArgs  ["f"]     = "bodies file name to read, do not use with -n "
	                     "(you can put 'data/in/np1/in.testcase1.0.dat').";
	reqArgs2 ["i"]     = "nIterations";

	faculArgs["v"]     = "";
	docArgs  ["v"]     = "activate verbose mode.";
	faculArgs["w"]     = "outputFileName";
	docArgs  ["w"]     = "base name of body file(s) to write (you can put 'data/out/out').";
	faculArgs["h"]     = "";
	docArgs  ["h"]     = "display this help.";
	faculArgs["-help"] = "";
	docArgs  ["-help"] = "display this help.";
	faculArgs["-dt"]   = "timeStep";
	docArgs  ["-dt"]   = "select a fixed time step in second.";
	faculArgs["-gs"]   = "";
	docArgs  ["-gs"]   = "enable geometry shader for visu, "
	                     "this is faster than the standard way but not all GPUs can support it.";
	faculArgs["-ww"]   = "winWidth";
	docArgs  ["-ww"]   = "width of the window in pixel (default is " + to_string(WinWidth) + ").";
	faculArgs["-wh"]   = "winHeight";
	docArgs  ["-wh"]   = "height of the window in pixel (default is " + to_string(WinHeight) + ").";
	faculArgs["-nv"]   = "";
	docArgs  ["-nv"]   = "no visualization (disable visu).";

	if(argsReader.parseArguments(reqArgs1, faculArgs))
	{
		NBodies       = stoi(argsReader.getArgument("n"));
		NIterations   = stoi(argsReader.getArgument("i"));
		InputFileName = "";
	}
	else if(argsReader.parseArguments(reqArgs2, faculArgs))
	{
		InputFileName = argsReader.getArgument("f");
		NIterations   = stoi(argsReader.getArgument("i"));
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
		OutputBaseName = argsReader.getArgument("w");
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
}

string strDate(TYPE timestamp)
{
	unsigned int days;
	unsigned int hours;
	unsigned int minutes;
	TYPE rest;

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

int main(int argc, char** argv)
{
	Perf perfIte, perfTotal;

	// read arguments from command line
	// usage: ./nbody -f fileName -i nIterations [-v] [-w] ...
	// usage: ./nbody -n nBodies  -i nIterations [-v] [-w] ...
	argsReader(argc, argv);

	// create plan by reading file or generate bodies randomly
	Space<TYPE> *space;
	if(InputFileName.empty())
		space = new Space<TYPE>(NBodies);
	else
		space = new Space<TYPE>(InputFileName);
	NBodies = space->getNBodies();

	// compute MB used for this simulation
	float Mbytes = (11 * sizeof(TYPE) * NBodies) / 1024.f / 1024.f;

	// compute flops per iteration
	float flopsPerIte = (float) NBodies * (float) (NBodies * 17);

	// display simulation configuration
	cout << "N-body simulation started !" << endl;
	cout << "---------------------------" << endl;
	if(!InputFileName.empty())
		cout << "  -> inputFileName    : " << InputFileName                       << endl;
	else
		cout << "  -> random mode      : enable"                                  << endl;
	if(!OutputBaseName.empty())
		cout << "  -> outputFileName(s): " << OutputBaseName << ".*.dat"          << endl;
	cout <<     "  -> nBodies          : " << NBodies                             << endl;
	cout <<     "  -> nIterations      : " << NIterations                         << endl;
	cout <<     "  -> verbose          : " << Verbose                             << endl;
#ifdef NBODY_DOUBLE
	cout <<     "  -> precision        : double"                                  << endl;
#else
	cout <<     "  -> precision        : simple"                                  << endl;
#endif
	cout <<     "  -> mem. used        : " << Mbytes << " MB"                     << endl;
	cout <<     "  -> geometry shader  : " << ((GSEnable) ? "enable" : "disable") << endl << endl;

	// initialize visualization of bodies (with spheres in space)
	OGLSpheresVisu<TYPE> *visu;
	if(VisuEnable)
	{
		if(GSEnable) // geometry shader = better performances on dedicated GPUs
			visu = new OGLSpheresVisuGS<TYPE>("N-body", WinWidth, WinHeight,
											  space->positions.x, space->positions.y, space->positions.z,
											  space->radiuses,
											  NBodies);
		else
			visu = new OGLSpheresVisuInst<TYPE>("N-body", WinWidth, WinHeight,
												space->positions.x, space->positions.y, space->positions.z,
												space->radiuses,
												NBodies);
		cout << endl;
	}
	else
		visu = new OGLSpheresVisuNo<TYPE>();

	cout << "Simulation started..." << endl;

	// write initial bodies into file
	if(!OutputBaseName.empty())
	{
		std::string outputFileName = OutputBaseName + ".0.dat";
		space->writeIntoFile(outputFileName);
	}

	// constant timestep (easier for the visualization)
	space->setDtConstant(Dt);

	TYPE physicTime = 0.0;
	unsigned long iIte;
	for(iIte = 1; iIte <= NIterations && !visu->windowShouldClose(); iIte++)
	{
		// refresh display in OpenGL window
		visu->refreshDisplay();

		perfIte.start();
		//-----------------------------//
		//-- Simulation computations --//
		space->iComputeBodiesAcceleration();
		space->findTimeStep();
		space->updateBodiesPositionAndSpeed();
		//-- Simulation computations --//
		//-----------------------------//
		perfIte.stop();
		perfTotal += perfIte;

		physicTime += space->getDt();

		if(Verbose)
			cout << "Processing step " << iIte << " took " << perfIte.getElapsedTime() << " ms "
				 << "(" << perfIte.getGflops(flopsPerIte)  << " Gflop/s), "
				 << "physic time: " << strDate(physicTime) << endl;

		// write iteration results into file
		if(!OutputBaseName.empty())
		{
			std::string outputFileName = OutputBaseName + "." + to_string(iIte) + ".dat";
			space->writeIntoFile(outputFileName);
		}
	}
	cout << "Simulation ended." << endl << endl;

	cout << "Entire simulation took " << perfTotal.getElapsedTime() << " ms "
	     << "(" << perfTotal.getGflops(flopsPerIte * (iIte -1)) << " Gflop/s)" << endl;

	delete visu;
	delete space;

	return EXIT_SUCCESS;
}
