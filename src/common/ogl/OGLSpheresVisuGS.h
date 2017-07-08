/*!
 * \file    OGLSpheresVisuGS.h
 * \brief   Class for spheres visualization with geometry shader in OpenGL.
 * \author  A. Cassagne
 * \date    2014
 *
 * \section LICENSE
 * This file is under CC BY-NC-ND license (http://creativecommons.org/licenses/by-nc-nd/4.0/legalcode).
 *
 * \section DESCRIPTION
 * This is the traditional entry file for the code execution.
 */
#ifndef OGL_SPHERES_VISU_GS_H_
#define OGL_SPHERES_VISU_GS_H_

#include <map>
#include <string>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

#include "OGLSpheresVisu.h"

template <typename T = double>
class OGLSpheresVisuGS :  public OGLSpheresVisu<T> {
public:
	OGLSpheresVisuGS(const std::string winName,
	                 const int winWidth,
	                 const int winHeight,
	                 const T *positionsX,
	                 const T *positionsY,
	                 const T *positionsZ,
	                 const T *radius,
	                 const unsigned long nSpheres);

	virtual ~OGLSpheresVisuGS();

	void refreshDisplay();
};

#include "OGLSpheresVisuGS.hxx"

#endif /* OGL_SPHERES_VISU_GS_H_ */