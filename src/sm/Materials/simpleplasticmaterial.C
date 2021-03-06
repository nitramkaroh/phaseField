/*
 *
 *                 #####    #####   ######  ######  ###   ###
 *               ##   ##  ##   ##  ##      ##      ## ### ##
 *              ##   ##  ##   ##  ####    ####    ##  #  ##
 *             ##   ##  ##   ##  ##      ##      ##     ##
 *            ##   ##  ##   ##  ##      ##      ##     ##
 *            #####    #####   ##      ######  ##     ##
 *
 *
 *             OOFEM : Object Oriented Finite Element Code
 *
 *               Copyright (C) 1993 - 2013   Borek Patzak
 *
 *
 *
 *       Czech Technical University, Faculty of Civil Engineering,
 *   Department of Structural Mechanics, 166 29 Prague, Czech Republic
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "simpleplasticmaterial.h"
#include "gausspoint.h"
#include "floatmatrix.h"
#include "floatarray.h"
#include "intarray.h"
#include "mathfem.h"
#include "datastream.h"
#include "contextioerr.h"

namespace oofem {

  SimplePlasticMaterial :: SimplePlasticMaterial(int n, Domain *d) : MPlasticMaterial2(n, d), IsotropicHardeningMaterialExtensionInterface(d), IsotropicDamageMaterialExtensionInterface(d)
{
    //
    // constructor
    //
    kinematicHardeningFlag = isotropicHardeningFlag = 0;
    this->nsurf = 1;
}

SimplePlasticMaterial :: ~SimplePlasticMaterial()
{ }

IRResultType
SimplePlasticMaterial :: initializeFrom(InputRecord *ir)
{

 // Required by IR_GIVE_FIELD macro
    IRResultType result;
    // call the corresponding service of structural material
    result = MPlasticMaterial2 :: initializeFrom(ir);
    if ( result != IRRT_OK ) return result;
    kinematicHardeningFlag = false;

    result = IsotropicHardeningMaterialExtensionInterface :: initializeFrom(ir);
    if ( result != IRRT_OK ) return result;
    if( isoHardeningType != IHT_None || isoHardeningType != IHT_Unknown) {
      isotropicHardeningFlag = true;
    }


    result = IsotropicDamageMaterialExtensionInterface :: initializeFrom(ir);
    if ( result != IRRT_OK ) return result;
   
    
    return result;  

  
    
  
}


MaterialStatus *
SimplePlasticMaterial :: CreateStatus(GaussPoint *gp) const
{
    return new MPlasticMaterial2Status(1, this->giveDomain(), gp, this->giveSizeOfReducedHardeningVarsVector(gp));
}

int
SimplePlasticMaterial :: giveSizeOfFullHardeningVarsVector()
{
    /* Returns the size of hardening variables vector */
    int size = 0;

    if ( kinematicHardeningFlag ) {
        size += 6;                      /* size of full stress vector */
    }

    if ( isotropicHardeningFlag ) {
        size += 1;                      /* scalar value */
    }

    return size;
}

int
SimplePlasticMaterial :: giveSizeOfReducedHardeningVarsVector(GaussPoint *gp) const
{
    /* Returns the size of hardening variables vector */
    int size = 0;

    if ( kinematicHardeningFlag ) {
        size += StructuralMaterial :: giveSizeOfVoigtSymVector( gp->giveMaterialMode() );
    }

    if ( isotropicHardeningFlag ) {
        size += 1;                      /* scalar value */
    }

    return size;
}



void
SimplePlasticMaterial :: computeStrainHardeningVarsIncrement(FloatArray &answer, GaussPoint *gp,
                                             const FloatArray &stress, const FloatArray &dlambda,
                                             const FloatArray &dplasticStrain, const IntArray &activeConditionMaps)
{

  int size = this->giveSizeOfReducedHardeningVarsVector(gp);
  answer.resize(size);
  FloatArray strainSpaceHardeningVariables(size);
  strainSpaceHardeningVariables.zero();
  
    if ( this->kinematicHardeningFlag ) {
    }

    if ( isotropicHardeningFlag ) {
      FloatArray loadGradSigVec;
      //      this->computeReducedStressGradientVector(loadGradSigVec, loadFunction, 1, gp, stress, strainSpaceHardeningVariables);
      this->computeStressGradientVector(loadGradSigVec, loadFunction, 1, gp, stress, strainSpaceHardeningVariables);
      answer.at(size) = computeStressNorm(loadGradSigVec)*dlambda.at(1);
    }
}


void
SimplePlasticMaterial :: computeKGradientVector(FloatArray &answer, functType ftype, int isurf, GaussPoint *gp, FloatArray &fullStressVector,
                                const FloatArray &strainSpaceHardeningVariables)
{

  int size = this->giveSizeOfReducedHardeningVarsVector(gp);
    FloatArray reducedKinematicGrad;

    if ( !hasHardening() ) {
        answer.clear();
        return;
    }

    answer.resize(size);

    /* kinematic hardening variables first */
    if ( this->kinematicHardeningFlag ) {
    
    }

    if ( this->isotropicHardeningFlag ) {
        answer.at(size) = -this->giveIsotropicHardeningModulus(strainSpaceHardeningVariables.at(size));
    }
}

void
SimplePlasticMaterial :: computeReducedHardeningVarsSigmaGradient(FloatMatrix &answer, GaussPoint *gp, const IntArray &activeConditionMap,
                                                  const FloatArray &fullStressVector,
                                                  const FloatArray &strainSpaceHardeningVars,
                                                  const FloatArray &gamma)
{

    int rsize = StructuralMaterial :: giveSizeOfVoigtSymVector( gp->giveMaterialMode() );
    answer.resize(giveSizeOfReducedHardeningVarsVector(gp), rsize);
    answer.zero();  
}

void
SimplePlasticMaterial :: computeReducedHardeningVarsLamGradient(FloatMatrix &answer, GaussPoint *gp, int actSurf,
                                                const IntArray &activeConditionMap,
                                                const FloatArray &fullStressVector,
                                                const FloatArray &strainSpaceHardeningVars,
                                                const FloatArray &gamma)
{

   int size = this->giveSizeOfReducedHardeningVarsVector(gp);
    answer.resize(size, 1);

    if ( this->kinematicHardeningFlag ) {
    }

    if ( isotropicHardeningFlag ) {
      FloatArray loadGradSigVec;
      this->computeStressGradientVector(loadGradSigVec, loadFunction, actSurf, gp, fullStressVector, strainSpaceHardeningVars);
      double norm = computeStressNorm(loadGradSigVec);
      if(norm != 0) {
	answer.at(size,1) = norm;
      }

    }
  
}




void
SimplePlasticMaterial :: computeReducedSKGradientMatrix(FloatMatrix &gradientMatrix,  int i, GaussPoint *gp, const FloatArray &fullStressVector,
                                        const FloatArray &strainSpaceHardeningVariables)
{

 // something will be here for k1 vector
    int size = giveSizeOfReducedHardeningVarsVector(gp);
    FloatMatrix helpMat;
    gradientMatrix.resize(StructuralMaterial :: giveSizeOfVoigtSymVector( gp->giveMaterialMode() ), size);
    gradientMatrix.zero();

}


int
SimplePlasticMaterial :: hasHardening()
{
    return ( this->isotropicHardeningFlag );
}


    
double
SimplePlasticMaterial ::   computeDamage(GaussPoint *gp, const FloatArray &strainSpaceHardeningVariables, TimeStep *tStep)
{
  
      int size = giveSizeOfReducedHardeningVarsVector(gp);
      return giveDamageParam(strainSpaceHardeningVariables.at(size));


}

double
SimplePlasticMaterial :: compute_dDamage_dKappa(GaussPoint *gp, const FloatArray &strainSpaceHardeningVariables, TimeStep *tStep)
{
    int size = giveSizeOfReducedHardeningVarsVector(gp);
    return giveDamageParamPrime(strainSpaceHardeningVariables.at(size));


}




  
  

} // end namespace oofem
