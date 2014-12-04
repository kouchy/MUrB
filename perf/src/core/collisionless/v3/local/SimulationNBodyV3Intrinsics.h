/*!
 * \file    SimulationNBodyV3Intrinsics.h
 * \brief   Implementation of SimulationNBodyLocal with intrinsic function calls (n² computations).
 * \author  A. Cassagne
 * \date    2014
 *
 * \section LICENSE
 * This file is under CC BY-NC-ND license (http://creativecommons.org/licenses/by-nc-nd/4.0/legalcode).
 */
#ifndef SIMULATION_N_BODY_V3_INTRINSICS_H_
#define SIMULATION_N_BODY_V3_INTRINSICS_H_

#include <string>

#include "../../../../utils/myIntrinsicsPlusPlus.h"

#include "SimulationNBodyV3.h"

/*!
 * \class  SimulationNBodyV3Intrinsics
 * \brief  Implementation of SimulationNBodyLocal with intrinsic function calls (n² computations).
 *
 * \tparam T : Type.
 */
template <typename T = double>
class SimulationNBodyV3Intrinsics : public SimulationNBodyV3<T>
{
public:
	SimulationNBodyV3Intrinsics(const unsigned long nBodies, T softening = 0.035);
	SimulationNBodyV3Intrinsics(const std::string inputFileName, T softening = 0.035);
	virtual ~SimulationNBodyV3Intrinsics();

protected:
	virtual void initIteration();
	virtual void computeLocalBodiesAcceleration();

	static inline void computeAccelerationBetweenTwoBodies(const mipp::vec &rG,
	                                                       const mipp::vec &rSoftSquared,
	                                                       const mipp::vec &rqiX,
	                                                       const mipp::vec &rqiY,
	                                                       const mipp::vec &rqiZ,
	                                                             mipp::vec &raiX,
	                                                             mipp::vec &raiY,
	                                                             mipp::vec &raiZ,
	                                                             mipp::vec &rclosNeighi,
	                                                       const mipp::vec &rmj,
	                                                       const mipp::vec &rqjX,
	                                                       const mipp::vec &rqjY,
	                                                       const mipp::vec &rqjZ);
private:
	void init();
	void _computeLocalBodiesAcceleration();
};

#include "SimulationNBodyV3Intrinsics.hxx"

#endif /* SIMULATION_N_BODY_V3_INTRINSICS_H_ */
