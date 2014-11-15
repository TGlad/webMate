// Copyright (c) 2008 NaturalMotion.  All Rights Reserved.
// Not to be copied, adapted, modified, used, distributed, sold,
// licensed or commercially exploited in any manner without the
// written consent of NaturalMotion.  
//
// All non public elements of this software are the confidential
// information of NaturalMotion and may not be disclosed to any
// person nor used for any purpose not expressly approved by
// NaturalMotion in writing.
#include "VectorMapper.h"
#include "debugDraw.h"
using namespace NMP; 

// This requires a bit of getting used to, there are really 2 ways you can use this mapper
// 1. when you know the mapping is quite linear (ie a matrix), in this case, you can set the learnRate to low
// e.g. 0.1f and slowly it will learn a mapping that will eventually work for all 'from' vectors
// 2. when you expect that the mapping is non-linear or complicated. In this case you can set the learnRate to 
// high e.g. 1.f and the mapping will move and change every frame, but will be a good approximate mapping for
// the local area around the most recent vectors. 
void VectorMapper::updateMapping(const NMP::Vector3 &from, const NMP::Vector3 &to)
{
  // TDL this algorithm seems to be pretty solid, I'm not entirely certain about a couple of bits
  // these are labelled with a *
  NMP::Vector3 guessedTarget;
  m_mapping.transformVector(from, guessedTarget);
  NMP::Vector3 error = to - guessedTarget;

  NMP::Matrix34 outerProduct;
  float scale = 1.f / (from.dot(from) + m_translationWeight); // * this makes the learn rate independent of the scale of acc
  outerProduct.r[0] = scale*from.x * error;
  outerProduct.r[1] = scale*from.y * error;
  outerProduct.r[2] = scale*from.z * error;
  outerProduct.r[3] = scale*m_translationWeight * error; 
  outerProduct.scale(m_learnRate);
  m_mapping.add(outerProduct);
}
