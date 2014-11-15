// Copyright (c) 2008 NaturalMotion.  All Rights Reserved.
// Not to be copied, adapted, modified, used, distributed, sold,
// licensed or commercially exploited in any manner without the
// written consent of NaturalMotion.  
//
// All non public elements of this software are the confidential
// information of NaturalMotion and may not be disclosed to any
// person nor used for any purpose not expressly approved by
// NaturalMotion in writing.
#ifndef NM_VectorMapper_H
#define NM_VectorMapper_H
#include "euph2/Junction.h"

// A class designed to continuously approximate the mapping from one vector type to another
// e.g. from a force to the corresponding acceleration. The mapping is a matrix (which includes scale and doesn't assume its an orthonormal mapping).
// The mapper learns the mapping (matrix) from the variety of input pairs
// Example use:
//
// static VectorMapper mapper;
// void update()
// {
//   mapper.updateMapping(force, acceleration); // each vector pair will improve the approximation of the mapping matrix
// }
// NMP::Vector3 predictedAcceleration;
// mapper.m_mapping.transformVector(newForce, predictedAcceleration);
//
// References:
// a similar thing but with orthonormal mappings: http://en.wikipedia.org/wiki/Kabsch_algorithm
// associative matrix memory http://www.gatsby.ucl.ac.uk/~dayan/papers/varcov93.pdf
// keywords: delta rule feedback learning, least squares regression
class VectorMapper
{
public:
  VectorMapper(){ initialise(); }
  float m_learnRate; // 0 to 1
  float m_translationWeight; // reduce this if we want the translation part to learn slower compared to the 3x3 part (or 0 to not use a translation)
  void initialise(float learnRate = 0.1f, float translationWeight = 0.05f){ m_learnRate = learnRate; m_translationWeight = translationWeight; m_mapping.identity(); }
  void updateMapping(const NMP::Vector3 &from, const NMP::Vector3 &to);
  NMP::Matrix34 m_mapping;
};

#endif // NM_VectorMapper_H
