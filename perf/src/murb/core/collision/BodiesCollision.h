/*!
 * \file    BodiesCollision.h
 * \brief   Bodies container with collisions management.
 * \author  A. Cassagne
 * \date    2014
 *
 * \section LICENSE
 * This file is under CC BY-NC-ND license (http://creativecommons.org/licenses/by-nc-nd/4.0/legalcode).
 */
#ifndef BODIES_COLLISION_H_
#define BODIES_COLLISION_H_

#include <string>

#include "../../../common/core/Bodies.h"

/*!
 * \class  BodiesCollision
 * \brief  BodiesCollision class represents the physic data of each body (mass, radius, position and velocity) and manages collisions.
 *
 * \tparam T : Type.
 */
template <typename T = double>
class BodiesCollision : public Bodies<T>
{
public:
	BodiesCollision();
	BodiesCollision(const unsigned long n, const unsigned long randInit = 0);
	BodiesCollision(const std::string inputFileName);
	BodiesCollision(const BodiesCollision<T>& bodies);

	virtual ~BodiesCollision();

	void applyCollisions(std::vector<std::vector<unsigned long>> collisions);
};

#include "BodiesCollision.hxx"

#endif /* BODIES_COLLISION_H_ */