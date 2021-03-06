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

#include "metricstrainmastermaterial.h"
#include "isolinearelasticmaterial.h"
#include "gausspoint.h"
#include "floatmatrix.h"
#include "floatarray.h"
#include "intarray.h"
#include "stressvector.h"
#include "strainvector.h"
#include "mathfem.h"
#include "contextioerr.h"
#include "datastream.h"
#include "classfactory.h"

namespace oofem {
REGISTER_Material(MetricStrainMasterMaterial);

// constructor
MetricStrainMasterMaterial :: MetricStrainMasterMaterial(int n, Domain *d) : StructuralMaterial(n, d)
{
    slaveMat = 0;
}

// destructor
MetricStrainMasterMaterial :: ~MetricStrainMasterMaterial()
{ }


// reads the model parameters from the input file
IRResultType
MetricStrainMasterMaterial :: initializeFrom(InputRecord *ir)
{
    IRResultType result;                 // required by IR_GIVE_FIELD macro



    IR_GIVE_OPTIONAL_FIELD(ir, slaveMat, _IFT_MetricStrainMasterMaterial_slaveMat); // number of slave material
    IR_GIVE_OPTIONAL_FIELD(ir, m, _IFT_MetricStrainMasterMaterial_m); // type of Metric strain tensor

    return IRRT_OK;
}

// creates a new material status  corresponding to this class
MaterialStatus *
MetricStrainMasterMaterial :: CreateStatus(GaussPoint *gp) const
{
    MetricStrainMasterMaterialStatus *status;
    status = new MetricStrainMasterMaterialStatus(1, this->giveDomain(), gp, slaveMat);
    return status;
}


void
MetricStrainMasterMaterial :: giveFirstPKStressVector_3d(FloatArray &answer, GaussPoint *gp, const FloatArray &vF, TimeStep *tStep)
{
    MetricStrainMasterMaterialStatus *status = static_cast< MetricStrainMasterMaterialStatus * >( this->giveStatus(gp) );
    this->initTempStatus(gp);
    StructuralMaterial *sMat = dynamic_cast< StructuralMaterial * >( domain->giveMaterial(slaveMat) );
    if ( sMat == NULL ) {
      OOFEM_WARNING("material %d has no Structural support", slaveMat);
      return;
    }

    FloatArray vEm, vSm;
    FloatMatrix invF,F, C, invC,I, Em, Sm, P, T;
    I.resize(3,3);
    I.beUnitMatrix();
    F.beMatrixForm(vF);
    invF.beInverseOf(F);
    // compute Green-Lagrange tensor = metric tensor and its inverse
    C.beTProductOf(F,F);
    invC.beInverseOf(C);
    // cumltiply C, invC and I by m coef.
    C.times((m + 2.)/8.);
    invC.times((m-2.)/8.);
    I.times(-m/4.);
    // construct metric strain 
    Em = C;
    Em.add(I);
    Em.add(invC);
    // convert metric strain from matrix to vector form
    vEm.beSymVectorFormOfStrain(Em);
    // compute stress work conjugated to metric strain
    sMat->giveRealStressVector_3d(vSm, status->giveSlaveGaussPoint(), vEm, tStep);
    // convert metric stress to matrix form
    Sm.beMatrixForm(vSm);
    // compute transformation matrix between the metric stress and 1PK sterss
    this->compute_Sm_2_P_TransformationMatrix(T, F, invF);
    // compute 1PK stress
    answer.beProductOf(T,vSm);
    // store 1PK stress into status
    status->letTempStressVectorBe(vSm);
    status->letPVectorBe(answer);
    // store F
    status->letTempFVectorBe(vF);


}


void 
MetricStrainMasterMaterial :: compute_Sm_2_P_TransformationMatrix(FloatMatrix &answer, FloatMatrix &F)
{
  FloatMatrix invF;
  invF.beInverseOf(F);
  this->compute_Sm_2_P_TransformationMatrix(answer,F,invF);

}

 
void 
MetricStrainMasterMaterial :: compute_Sm_2_P_TransformationMatrix(FloatMatrix &answer, FloatMatrix &F, FloatMatrix &invF)
{
  
  double a = ( 2. + m ) / 8.;
  double b = ( 2. - m ) / 8.;

  answer.resize(9,6);
  FloatMatrix invC;
  invC.beProductTOf(invF,invF);

  answer.at(1,1) = 2*F.at(1,1)*a + 2*b*invC.at(1,1)*invF.at(1,1); 
  answer.at(1,2) = 2*b*invC.at(1,2)*invF.at(2,1); 
  answer.at(1,3) = 2*b*invC.at(1,3)*invF.at(3,1); 
  answer.at(1,4) = b*(invC.at(1,3)*invF.at(2,1) + invC.at(1,2)*invF.at(3,1)) +b*(invC.at(1,3)*invF.at(2,1) + invC.at(1,2)*invF.at(3,1));
  answer.at(1,5) = F.at(1,3)*a + b*(invC.at(1,3)*invF.at(1,1) + invC.at(1,1)*invF.at(3,1)) + F.at(1,3)*a + b*(invC.at(1,3)*invF.at(1,1) + invC.at(1,1)*invF.at(3,1));
  answer.at(1,6) = F.at(1,2)*a + b*(invC.at(1,2)*invF.at(1,1) + invC.at(1,1)*invF.at(2,1)) +F.at(1,2)*a + b*(invC.at(1,2)*invF.at(1,1) + invC.at(1,1)*invF.at(2,1));
  
  
  answer.at(2,1) = 2*b*invF.at(1,2)*invC.at(2,1); 
  answer.at(2,2) = 2*F.at(2,2)*a + 2*b*invC.at(2,2)*invF.at(2,2);
  answer.at(2,3) = 2*b*invC.at(2,3)*invF.at(3,2);
  answer.at(2,4) = F.at(2,3)*a + b*(invC.at(2,3)*invF.at(2,2) + invC.at(2,2)*invF.at(3,2)) + F.at(2,3)*a + b*(invC.at(2,3)*invF.at(2,2) + invC.at(2,2)*invF.at(3,2));
  answer.at(2,5) = b*(invF.at(1,2)*invC.at(2,3) + invC.at(2,1)*invF.at(3,2)) + b*(invF.at(1,2)*invC.at(2,3) + invC.at(2,1)*invF.at(3,2));
  answer.at(2,6) = F.at(2,1)*a + b*(invF.at(1,2)*invC.at(2,2) + invC.at(2,1)*invF.at(2,2)) + F.at(2,1)*a + b*(invF.at(1,2)*invC.at(2,2) + invC.at(2,1)*invF.at(2,2));
  
  
  answer.at(3,1) = 2*b*invF.at(1,3)*invC.at(3,1);
  answer.at(3,2) = 2*b*invF.at(2,3)*invC.at(3,2); 
  answer.at(3,3) = 2*F.at(3,3)*a + 2*b*invC.at(3,3)*invF.at(3,3);
  answer.at(3,4) = F.at(3,2)*a + b*(invF.at(2,3)*invC.at(3,3) + invC.at(3,2)*invF.at(3,3)) + F.at(3,2)*a + b*(invF.at(2,3)*invC.at(3,3) + invC.at(3,2)*invF.at(3,3));
  answer.at(3,5) = F.at(3,1)*a + b*(invF.at(1,3)*invC.at(3,3) + invC.at(3,1)*invF.at(3,3)) + F.at(3,1)*a + b*(invF.at(1,3)*invC.at(3,3) + invC.at(3,1)*invF.at(3,3));
  answer.at(3,6) = b*(invF.at(1,3)*invC.at(3,2) + invF.at(2,3)*invC.at(3,1)) + b*(invF.at(1,3)*invC.at(3,2) + invF.at(2,3)*invC.at(3,1));
  
  answer.at(4,1) = 2*b*invF.at(1,2)*invC.at(3,1); 
  answer.at(4,2) = 2*b*invF.at(2,2)*invC.at(3,2);
  answer.at(4,3) = 2*F.at(2,3)*a + 2*b*invC.at(3,3)*invF.at(3,2);
  answer.at(4,4) = F.at(2,2)*a + b*(invF.at(2,2)*invC.at(3,3) + invC.at(3,2)*invF.at(3,2)) + F.at(2,2)*a + b*(invF.at(2,2)*invC.at(3,3) + invC.at(3,2)*invF.at(3,2));;
  answer.at(4,5) = F.at(2,1)*a + b*(invF.at(1,2)*invC.at(3,3) + invC.at(3,1)*invF.at(3,2)) + F.at(2,1)*a + b*(invF.at(1,2)*invC.at(3,3) + invC.at(3,1)*invF.at(3,2));
  answer.at(4,6) = b*(invF.at(1,2)*invC.at(3,2) + invF.at(2,2)*invC.at(3,1)) + b*(invF.at(1,2)*invC.at(3,2) + invF.at(2,2)*invC.at(3,1));
  
  
  answer.at(5,1) = 2*b*invF.at(1,1)*invC.at(3,1);
  answer.at(5,2) = 2*b*invF.at(2,1)*invC.at(3,2);
  answer.at(5,3) = 2*F.at(1,3)*a + 2*b*invC.at(3,3)*invF.at(3,1);
  answer.at(5,4) = F.at(1,2)*a + b*(invF.at(2,1)*invC.at(3,3) + invC.at(3,2)*invF.at(3,1)) + F.at(1,2)*a + b*(invF.at(2,1)*invC.at(3,3) + invC.at(3,2)*invF.at(3,1));
  answer.at(5,5) = F.at(1,1)*a + b*(invF.at(1,1)*invC.at(3,3) + invC.at(3,1)*invF.at(3,1)) + F.at(1,1)*a + b*(invF.at(1,1)*invC.at(3,3) + invC.at(3,1)*invF.at(3,1));
  answer.at(5,6) = b*(invF.at(1,1)*invC.at(3,2) + invF.at(2,1)*invC.at(3,1)) + b*(invF.at(1,1)*invC.at(3,2) + invF.at(2,1)*invC.at(3,1));
  
  
  answer.at(6,1) = 2*b*invF.at(1,1)*invC.at(2,1) ;
  answer.at(6,2) = 2*F.at(1,2)*a + 2*b*invC.at(2,2)*invF.at(2,1) ;
  answer.at(6,3) = 2*b*invC.at(2,3)*invF.at(3,1) ;
  answer.at(6,4) = F.at(1,3)*a + b*(invC.at(2,3)*invF.at(2,1) + invC.at(2,2)*invF.at(3,1))  +  F.at(1,3)*a + b*(invC.at(2,3)*invF.at(2,1) + invC.at(2,2)*invF.at(3,1)) ;
  answer.at(6,5) = b*(invF.at(1,1)*invC.at(2,3) + invC.at(2,1)*invF.at(3,1)) + b*(invF.at(1,1)*invC.at(2,3) + invC.at(2,1)*invF.at(3,1)) ;
  answer.at(6,6) = F.at(1,1)*a + b*(invF.at(1,1)*invC.at(2,2) + invC.at(2,1)*invF.at(2,1)) + F.at(1,1)*a + b*(invF.at(1,1)*invC.at(2,2) + invC.at(2,1)*invF.at(2,1)) ;
  
 
  answer.at(7,1) = 2*b*invF.at(1,3)*invC.at(2,1) ;
  answer.at(7,2) = 2*F.at(3,2)*a + 2*b*invC.at(2,2)*invF.at(2,3) ;
  answer.at(7,3) = 2*b*invC.at(2,3)*invF.at(3,3) ;
  answer.at(7,4) = F.at(3,3)*a + b*(invC.at(2,3)*invF.at(2,3) + invC.at(2,2)*invF.at(3,3)) + F.at(3,3)*a + b*(invC.at(2,3)*invF.at(2,3) + invC.at(2,2)*invF.at(3,3)) ;
  answer.at(7,5) = b*(invF.at(1,3)*invC.at(2,3) + invC.at(2,1)*invF.at(3,3)) + b*(invF.at(1,3)*invC.at(2,3) + invC.at(2,1)*invF.at(3,3)) ;
  answer.at(7,6) = F.at(3,1)*a + b*(invF.at(1,3)*invC.at(2,2) + invC.at(2,1)*invF.at(2,3)) + F.at(3,1)*a + b*(invF.at(1,3)*invC.at(2,2) + invC.at(2,1)*invF.at(2,3));
  
  
  answer.at(8,1) = 2*F.at(3,1)*a + 2*b*invC.at(1,1)*invF.at(1,3) ;
  answer.at(8,2) = 2*b*invC.at(1,2)*invF.at(2,3) ;
  answer.at(8,3) = 2*b*invC.at(1,3)*invF.at(3,3) ;
  answer.at(8,4) = b*(invC.at(1,3)*invF.at(2,3) + invC.at(1,2)*invF.at(3,3)) + b*(invC.at(1,3)*invF.at(2,3) + invC.at(1,2)*invF.at(3,3)) ;
  answer.at(8,5) = F.at(3,3)*a + b*(invC.at(1,3)*invF.at(1,3) + invC.at(1,1)*invF.at(3,3)) + F.at(3,3)*a + b*(invC.at(1,3)*invF.at(1,3) + invC.at(1,1)*invF.at(3,3)) ;
  answer.at(8,6) = F.at(3,2)*a + b*(invC.at(1,2)*invF.at(1,3) + invC.at(1,1)*invF.at(2,3)) + F.at(3,2)*a + b*(invC.at(1,2)*invF.at(1,3) + invC.at(1,1)*invF.at(2,3));
  
  
  answer.at(9,1) = 2*F.at(2,1)*a + 2*b*invC.at(1,1)*invF.at(1,2) ;
  answer.at(9,2) = 2*b*invC.at(1,2)*invF.at(2,2) ;
  answer.at(9,3) = 2*b*invC.at(1,3)*invF.at(3,2) ;
  answer.at(9,4) = b*(invC.at(1,3)*invF.at(2,2) + invC.at(1,2)*invF.at(3,2)) + b*(invC.at(1,3)*invF.at(2,2) + invC.at(1,2)*invF.at(3,2)) ;
  answer.at(9,5) = F.at(2,3)*a + b*(invC.at(1,3)*invF.at(1,2) + invC.at(1,1)*invF.at(3,2)) + F.at(2,3)*a + b*(invC.at(1,3)*invF.at(1,2) + invC.at(1,1)*invF.at(3,2)) ;
  answer.at(9,6) = F.at(2,2)*a + b*(invC.at(1,2)*invF.at(1,2) + invC.at(1,1)*invF.at(2,2)) + F.at(2,2)*a + b*(invC.at(1,2)*invF.at(1,2) + invC.at(1,1)*invF.at(2,2));
}


void
MetricStrainMasterMaterial :: give3dMaterialStiffnessMatrix_dPdF(FloatMatrix &answer, MatResponseMode mode, GaussPoint *gp, TimeStep *tStep)
{
    MetricStrainMasterMaterialStatus *status = static_cast< MetricStrainMasterMaterialStatus * >( this->giveStatus(gp) );
    FloatMatrix stiffness;  
    Material *mat;
    StructuralMaterial *sMat;
    mat = domain->giveMaterial(slaveMat);
    sMat = dynamic_cast< StructuralMaterial * >(mat);
    if ( sMat == NULL ) {
      OOFEM_WARNING("material %d has no Structural support", slaveMat);
      return;
    }
    // compute metric stiffness matrix dSm/dEm
    sMat->give3dMaterialStiffnessMatrix(stiffness, mode, status->giveSlaveGaussPoint(), tStep); 
    // transform dSm/dEm to dP/dF
    FloatArray vF = status->giveTempFVector();
    FloatArray vSm = status->giveTempStressVector();
    this->convert_dSmdEm_2_dPdF( answer, stiffness, vSm, vF, gp->giveMaterialMode() );
}





void
MetricStrainMasterMaterial :: convert_dSmdEm_2_dPdF(FloatMatrix &answer, const FloatMatrix &dSdE, FloatArray &vSm, FloatArray &vF, MaterialMode matMode)
{   

  double a,b;
  FloatMatrix F, T, invC, invF, Sm, iFtSmiF, iCSmiF, junk;


  a = ( 2. + m ) / 4.;
  b = ( 2. - m ) / 4.;
  
  F.beMatrixForm(vF);
  Sm.beMatrixForm(vSm);

  // compute first contribution -  P:D:P
  this->compute_Sm_2_P_TransformationMatrix(T, F);    
  junk.beProductOf(T,dSdE);
  answer.beProductTOf(junk,T);

  invF.beInverseOf(F);
  invC.beProductTOf(invF,invF);
  junk.beTProductOf(invF,Sm);
  iFtSmiF.beProductOf(junk,invF);  
  junk.beProductOf(invC,Sm);
  iCSmiF.beProductOf(junk,invF);


  answer.at(1,1) -= b* (iFtSmiF.at(1,1)*invC.at(1,1) +  2.*iCSmiF.at(1,1)*invF.at(1,1)  ); 
  answer.at(1,2) -= b* (iFtSmiF.at(1,2)*invC.at(1,2) +   iCSmiF.at(1,2)*invF.at(2,1) + iCSmiF.at(2,1)*invF.at(1,2) );
  answer.at(1,3) -= b* (iFtSmiF.at(1,3)*invC.at(1,3) +   iCSmiF.at(1,3)*invF.at(3,1) + iCSmiF.at(3,1)*invF.at(1,3) ); 
  answer.at(1,4) -= b* (iFtSmiF.at(1,2)*invC.at(1,3) +   iCSmiF.at(1,2)*invF.at(3,1) + iCSmiF.at(3,1)*invF.at(1,2) ); 
  answer.at(1,5) -= b* (iFtSmiF.at(1,1)*invC.at(1,3) +   iCSmiF.at(1,1)*invF.at(3,1) + iCSmiF.at(3,1)*invF.at(1,1) ); 
  answer.at(1,6) -= b* (iFtSmiF.at(1,1)*invC.at(1,2) +   iCSmiF.at(1,1)*invF.at(2,1) + iCSmiF.at(2,1)*invF.at(1,1) ); 
  answer.at(1,7) -= b* (iFtSmiF.at(1,3)*invC.at(1,2) +   iCSmiF.at(1,3)*invF.at(2,1) + iCSmiF.at(2,1)*invF.at(1,3) ); 
  answer.at(1,8) -= b* (iFtSmiF.at(1,3)*invC.at(1,1) +    iCSmiF.at(1,1)*invF.at(1,3) + iCSmiF.at(1,3)*invF.at(1,1)); 
  answer.at(1,9) -= b* (iFtSmiF.at(1,2)*invC.at(1,1) +   iCSmiF.at(1,1)*invF.at(1,2) + iCSmiF.at(1,2)*invF.at(1,1) ); 
 
  answer.at(2,1) -= b* (iFtSmiF.at(2,1)*invC.at(2,1) +    iCSmiF.at(1,2)*invF.at(2,1) + iCSmiF.at(2,1)*invF.at(1,2)); 
  answer.at(2,2) -= b* (iFtSmiF.at(2,2)*invC.at(2,2) +    2.*iCSmiF.at(2,2)*invF.at(2,2)); 
  answer.at(2,3) -= b* (iFtSmiF.at(2,3)*invC.at(2,3) +    iCSmiF.at(2,3)*invF.at(3,2) + iCSmiF.at(3,2)*invF.at(2,3));
  answer.at(2,4) -= b* (iFtSmiF.at(2,2)*invC.at(2,3) +    iCSmiF.at(2,2)*invF.at(3,2) + iCSmiF.at(3,2)*invF.at(2,2)); 
  answer.at(2,5) -= b* (iFtSmiF.at(2,1)*invC.at(2,3) +    iCSmiF.at(2,1)*invF.at(3,2) + iCSmiF.at(3,2)*invF.at(2,1)); 
  answer.at(2,6) -= b* (iFtSmiF.at(2,1)*invC.at(2,2) +    iCSmiF.at(2,1)*invF.at(2,2) + iCSmiF.at(2,2)*invF.at(2,1)); 
  answer.at(2,7) -= b* (iFtSmiF.at(2,3)*invC.at(2,2) +    iCSmiF.at(2,2)*invF.at(2,3) + iCSmiF.at(2,3)*invF.at(2,2)); 
  answer.at(2,8) -= b* (iFtSmiF.at(2,3)*invC.at(2,1) +    iCSmiF.at(1,2)*invF.at(2,3) + iCSmiF.at(2,3)*invF.at(1,2)); 
  answer.at(2,9) -= b* (iFtSmiF.at(2,2)*invC.at(2,1) +    iCSmiF.at(1,2)*invF.at(2,2) + iCSmiF.at(2,2)*invF.at(1,2)); 
 
  answer.at(3,1) -= b* (iFtSmiF.at(3,1)*invC.at(3,1) +    iCSmiF.at(1,3)*invF.at(3,1) + iCSmiF.at(3,1)*invF.at(1,3)); 
  answer.at(3,2) -= b* (iFtSmiF.at(3,2)*invC.at(3,2) +    iCSmiF.at(2,3)*invF.at(3,2) + iCSmiF.at(3,2)*invF.at(2,3)); 
  answer.at(3,3) -= b* (iFtSmiF.at(3,3)*invC.at(3,3) +    2.*iCSmiF.at(3,3)*invF.at(3,3)); 
  answer.at(3,4) -= b* (iFtSmiF.at(3,2)*invC.at(3,3) +    iCSmiF.at(3,2)*invF.at(3,3) + iCSmiF.at(3,3)*invF.at(3,2)); 
  answer.at(3,5) -= b* (iFtSmiF.at(3,1)*invC.at(3,3) +    iCSmiF.at(3,1)*invF.at(3,3) + iCSmiF.at(3,3)*invF.at(3,1)); 
  answer.at(3,6) -= b* (iFtSmiF.at(3,1)*invC.at(3,2) +    iCSmiF.at(2,3)*invF.at(3,1) + iCSmiF.at(3,1)*invF.at(2,3)); 
  answer.at(3,7) -= b* (iFtSmiF.at(3,3)*invC.at(3,2) +    iCSmiF.at(2,3)*invF.at(3,3) + iCSmiF.at(3,3)*invF.at(2,3)); 
  answer.at(3,8) -= b* (iFtSmiF.at(3,3)*invC.at(3,1) +    iCSmiF.at(1,3)*invF.at(3,3) + iCSmiF.at(3,3)*invF.at(1,3)); 
  answer.at(3,9) -= b* (iFtSmiF.at(3,2)*invC.at(3,1) +    iCSmiF.at(1,3)*invF.at(3,2) + iCSmiF.at(3,2)*invF.at(1,3));
 
 answer.at(4,1) -= b* (iFtSmiF.at(2,1)*invC.at(3,1) +    iCSmiF.at(1,2)*invF.at(3,1) + iCSmiF.at(3,1)*invF.at(1,2)); 
 answer.at(4,2) -= b* (iFtSmiF.at(2,2)*invC.at(3,2) +    iCSmiF.at(2,2)*invF.at(3,2) + iCSmiF.at(3,2)*invF.at(2,2)); 
 answer.at(4,3) -= b* (iFtSmiF.at(2,3)*invC.at(3,3) +    iCSmiF.at(3,2)*invF.at(3,3) + iCSmiF.at(3,3)*invF.at(3,2)); 
 answer.at(4,4) -= b* (iFtSmiF.at(2,2)*invC.at(3,3) +    2.*iCSmiF.at(3,2)*invF.at(3,2)); 
 answer.at(4,5) -= b* (iFtSmiF.at(2,1)*invC.at(3,3) +    iCSmiF.at(3,1)*invF.at(3,2) + iCSmiF.at(3,2)*invF.at(3,1)); 
 answer.at(4,6) -= b* (iFtSmiF.at(2,1)*invC.at(3,2) +    iCSmiF.at(2,2)*invF.at(3,1) + iCSmiF.at(3,1)*invF.at(2,2)); 
 answer.at(4,7) -= b* (iFtSmiF.at(2,3)*invC.at(3,2) +    iCSmiF.at(2,2)*invF.at(3,3) + iCSmiF.at(3,3)*invF.at(2,2)); 
 answer.at(4,8) -= b* (iFtSmiF.at(2,3)*invC.at(3,1) +    iCSmiF.at(1,2)*invF.at(3,3) + iCSmiF.at(3,3)*invF.at(1,2)); 
 answer.at(4,9) -= b* (iFtSmiF.at(2,2)*invC.at(3,1) +    iCSmiF.at(1,2)*invF.at(3,2) + iCSmiF.at(3,2)*invF.at(1,2));
 
 
 answer.at(5,1) -= b* (iFtSmiF.at(1,1)*invC.at(3,1) +    iCSmiF.at(1,1)*invF.at(3,1) + iCSmiF.at(3,1)*invF.at(1,1)); 
 answer.at(5,2) -= b* (iFtSmiF.at(1,2)*invC.at(3,2) +    iCSmiF.at(2,1)*invF.at(3,2) + iCSmiF.at(3,2)*invF.at(2,1)); 
 answer.at(5,3) -= b* (iFtSmiF.at(1,3)*invC.at(3,3) +    iCSmiF.at(3,1)*invF.at(3,3) + iCSmiF.at(3,3)*invF.at(3,1)); 
 answer.at(5,4) -= b* (iFtSmiF.at(1,2)*invC.at(3,3) +    iCSmiF.at(3,1)*invF.at(3,2) + iCSmiF.at(3,2)*invF.at(3,1)); 
 answer.at(5,5) -= b* (iFtSmiF.at(1,1)*invC.at(3,3) +    2.*iCSmiF.at(3,1)*invF.at(3,1)); 
 answer.at(5,6) -= b* (iFtSmiF.at(1,1)*invC.at(3,2) +    iCSmiF.at(2,1)*invF.at(3,1) + iCSmiF.at(3,1)*invF.at(2,1)); 
 answer.at(5,7) -= b* (iFtSmiF.at(1,3)*invC.at(3,2) +    iCSmiF.at(2,1)*invF.at(3,3) + iCSmiF.at(3,3)*invF.at(2,1)); 
 answer.at(5,8) -= b* (iFtSmiF.at(1,3)*invC.at(3,1) +    iCSmiF.at(1,1)*invF.at(3,3) + iCSmiF.at(3,3)*invF.at(1,1)); 
 answer.at(5,9) -= b* (iFtSmiF.at(1,2)*invC.at(3,1) +    iCSmiF.at(1,1)*invF.at(3,2) + iCSmiF.at(3,2)*invF.at(1,1)); 

 answer.at(6,1) -= b* (iFtSmiF.at(1,1)*invC.at(2,1) +    iCSmiF.at(1,1)*invF.at(2,1) + iCSmiF.at(2,1)*invF.at(1,1)); 
 answer.at(6,2) -= b* (iFtSmiF.at(1,2)*invC.at(2,2) +    iCSmiF.at(2,1)*invF.at(2,2) + iCSmiF.at(2,2)*invF.at(2,1)); 
 answer.at(6,3) -= b* (iFtSmiF.at(1,3)*invC.at(2,3) +    iCSmiF.at(2,3)*invF.at(3,1) + iCSmiF.at(3,1)*invF.at(2,3)); 
 answer.at(6,4) -= b* (iFtSmiF.at(1,2)*invC.at(2,3) +    iCSmiF.at(2,2)*invF.at(3,1) + iCSmiF.at(3,1)*invF.at(2,2)); 
 answer.at(6,5) -= b* (iFtSmiF.at(1,1)*invC.at(2,3) +    iCSmiF.at(2,1)*invF.at(3,1) + iCSmiF.at(3,1)*invF.at(2,1)); 
 answer.at(6,6) -= b* (iFtSmiF.at(1,1)*invC.at(2,2) +    2.*iCSmiF.at(2,1)*invF.at(2,1)); 
 answer.at(6,7) -= b* (iFtSmiF.at(1,3)*invC.at(2,2) +    iCSmiF.at(2,1)*invF.at(2,3) + iCSmiF.at(2,3)*invF.at(2,1)); 
 answer.at(6,8) -= b* (iFtSmiF.at(1,3)*invC.at(2,1) +    iCSmiF.at(1,1)*invF.at(2,3) + iCSmiF.at(2,3)*invF.at(1,1)); 
 answer.at(6,9) -= b* (iFtSmiF.at(1,2)*invC.at(2,1) +    iCSmiF.at(1,1)*invF.at(2,2) + iCSmiF.at(2,2)*invF.at(1,1)); 

 answer.at(7,1) -= b* (iFtSmiF.at(3,1)*invC.at(2,1) +    iCSmiF.at(1,3)*invF.at(2,1) + iCSmiF.at(2,1)*invF.at(1,3)); 
 answer.at(7,2) -= b* (iFtSmiF.at(3,2)*invC.at(2,2) +    iCSmiF.at(2,2)*invF.at(2,3) + iCSmiF.at(2,3)*invF.at(2,2)); 
 answer.at(7,3) -= b* (iFtSmiF.at(3,3)*invC.at(2,3) +    iCSmiF.at(2,3)*invF.at(3,3) + iCSmiF.at(3,3)*invF.at(2,3)); 
 answer.at(7,4) -= b* (iFtSmiF.at(3,2)*invC.at(2,3) +    iCSmiF.at(2,2)*invF.at(3,3) + iCSmiF.at(3,3)*invF.at(2,2)); 
 answer.at(7,5) -= b* (iFtSmiF.at(3,1)*invC.at(2,3) +    iCSmiF.at(2,1)*invF.at(3,3) + iCSmiF.at(3,3)*invF.at(2,1)); 
 answer.at(7,6) -= b* (iFtSmiF.at(3,1)*invC.at(2,2) +    iCSmiF.at(2,1)*invF.at(2,3) + iCSmiF.at(2,3)*invF.at(2,1)); 
 answer.at(7,7) -= b* (iFtSmiF.at(3,3)*invC.at(2,2) +    2.*iCSmiF.at(2,3)*invF.at(2,3)); 
 answer.at(7,8) -= b* (iFtSmiF.at(3,3)*invC.at(2,1) +    iCSmiF.at(1,3)*invF.at(2,3) + iCSmiF.at(2,3)*invF.at(1,3)); 
 answer.at(7,9) -= b* (iFtSmiF.at(3,2)*invC.at(2,1) +    iCSmiF.at(1,3)*invF.at(2,2) + iCSmiF.at(2,2)*invF.at(1,3)); 


 answer.at(8,1) -= b* (iFtSmiF.at(3,1)*invC.at(1,1) +    iCSmiF.at(1,1)*invF.at(1,3) + iCSmiF.at(1,3)*invF.at(1,1)); 
 answer.at(8,2) -= b* (iFtSmiF.at(3,2)*invC.at(1,2) +    iCSmiF.at(1,2)*invF.at(2,3) + iCSmiF.at(2,3)*invF.at(1,2)); 
 answer.at(8,3) -= b* (iFtSmiF.at(3,3)*invC.at(1,3) +    iCSmiF.at(1,3)*invF.at(3,3) + iCSmiF.at(3,3)*invF.at(1,3)); 
 answer.at(8,4) -= b* (iFtSmiF.at(3,2)*invC.at(1,3) +    iCSmiF.at(1,2)*invF.at(3,3) + iCSmiF.at(3,3)*invF.at(1,2)); 
 answer.at(8,5) -= b* (iFtSmiF.at(3,1)*invC.at(1,3) +    iCSmiF.at(1,1)*invF.at(3,3) + iCSmiF.at(3,3)*invF.at(1,1)); 
 answer.at(8,6) -= b* (iFtSmiF.at(3,1)*invC.at(1,2) +    iCSmiF.at(1,1)*invF.at(2,3) + iCSmiF.at(2,3)*invF.at(1,1)); 
 answer.at(8,7) -= b* (iFtSmiF.at(3,3)*invC.at(1,2) +    iCSmiF.at(1,3)*invF.at(2,3) + iCSmiF.at(2,3)*invF.at(1,3)); 
 answer.at(8,8) -= b* (iFtSmiF.at(3,3)*invC.at(1,1) +    2.*iCSmiF.at(1,3)*invF.at(1,3)); 
 answer.at(8,9) -= b* (iFtSmiF.at(3,2)*invC.at(1,1) +    iCSmiF.at(1,2)*invF.at(1,3) + iCSmiF.at(1,3)*invF.at(1,2)); 

 answer.at(9,1) -= b* (iFtSmiF.at(2,1)*invC.at(1,1) +    iCSmiF.at(1,1)*invF.at(1,2) + iCSmiF.at(1,2)*invF.at(1,1)); 
 answer.at(9,2) -= b* (iFtSmiF.at(2,2)*invC.at(1,2) +    iCSmiF.at(1,2)*invF.at(2,2) + iCSmiF.at(2,2)*invF.at(1,2)); 
 answer.at(9,3) -= b* (iFtSmiF.at(2,3)*invC.at(1,3) +    iCSmiF.at(1,3)*invF.at(3,2) + iCSmiF.at(3,2)*invF.at(1,3)); 
 answer.at(9,4) -= b* (iFtSmiF.at(2,2)*invC.at(1,3) +    iCSmiF.at(1,2)*invF.at(3,2) + iCSmiF.at(3,2)*invF.at(1,2)); 
 answer.at(9,5) -= b* (iFtSmiF.at(2,1)*invC.at(1,3) +    iCSmiF.at(1,1)*invF.at(3,2) + iCSmiF.at(3,2)*invF.at(1,1)); 
 answer.at(9,6) -= b* (iFtSmiF.at(2,1)*invC.at(1,2) +    iCSmiF.at(1,1)*invF.at(2,2) + iCSmiF.at(2,2)*invF.at(1,1)); 
 answer.at(9,7) -= b* (iFtSmiF.at(2,3)*invC.at(1,2) +    iCSmiF.at(1,3)*invF.at(2,2) + iCSmiF.at(2,2)*invF.at(1,3)); 
 answer.at(9,8) -= b* (iFtSmiF.at(2,3)*invC.at(1,1) +    iCSmiF.at(1,2)*invF.at(1,3) + iCSmiF.at(1,3)*invF.at(1,2)); 
 answer.at(9,9) -= b* (iFtSmiF.at(2,2)*invC.at(1,1) +    2.*iCSmiF.at(1,2)*invF.at(1,2));
															       

 answer.at(1,1) +=  a*Sm.at(1,1);
 answer.at(1,5) +=  a*Sm.at(1,3); 
 answer.at(1,6) +=  a*Sm.at(1,2);
 
 answer.at(2,2) +=  a*Sm.at(2,2);
 answer.at(2,4) +=  a*Sm.at(2,3);
 answer.at(2,9) +=  a*Sm.at(2,1);

 answer.at(3,3) +=  a*Sm.at(3,3);
 answer.at(3,2) +=  a*Sm.at(3,2);
 answer.at(3,1) +=  a*Sm.at(3,1); 
 
 answer.at(4,2) += a*Sm.at(3,2);
 answer.at(4,4) += a*Sm.at(3,3); 
 answer.at(4,9) += a*Sm.at(3,1);

 answer.at(5,1) +=  a*Sm.at(3,1);
 answer.at(5,5) +=  a*Sm.at(3,3);
 answer.at(5,6) +=  a*Sm.at(3,2);

 answer.at(6,1) +=  a*Sm.at(2,1);
 answer.at(6,5) +=  a*Sm.at(2,3);
 answer.at(6,6) +=  a*Sm.at(2,2); 

 answer.at(7,3) += a*Sm.at(2,3);
 answer.at(7,7) += a*Sm.at(2,2);
 answer.at(7,8) += a*Sm.at(2,1);

 answer.at(8,3) += a*Sm.at(1,3);
 answer.at(8,7) += a*Sm.at(1,2);
 answer.at(8,8) += a*Sm.at(1,1); 

 answer.at(9,2) += a*Sm.at(1,2); 
 answer.at(9,4) += a*Sm.at(1,3); 
 answer.at(9,9) += a*Sm.at(1,1); 

}











int
MetricStrainMasterMaterial :: giveIPValue(FloatArray &answer, GaussPoint *gp, InternalStateType type, TimeStep *tStep)
{
    MetricStrainMasterMaterialStatus *status = static_cast< MetricStrainMasterMaterialStatus * >( this->giveStatus(gp) );

    if ( type == IST_StressTensor ) {
        answer = status->giveStressVector();
        return 1;
    } else if ( type == IST_StrainTensor ) {
        answer = status->giveStrainVector();
        return 1;
    } else {
        Material *mat;
        StructuralMaterial *sMat;
        mat = domain->giveMaterial(slaveMat);
        sMat = dynamic_cast< StructuralMaterial * >(mat);
        if ( sMat == NULL ) {
            OOFEM_WARNING("material %d has no Structural support", slaveMat);
            return 0;
        }

        int result = sMat->giveIPValue(answer, gp, type, tStep);
        return result;
    }
}


//=============================================================================

MetricStrainMasterMaterialStatus :: MetricStrainMasterMaterialStatus(int n, Domain *d, GaussPoint *g, int s) : StructuralMaterialStatus(n, d, g), slaveMat(s)
{
  FloatArray coords  = {0.,0.,0.};
  slaveGp = new GaussPoint( NULL, 1, coords, 1.0, _3dMat);
  this->createSlaveStatus();
}


MetricStrainMasterMaterialStatus :: ~MetricStrainMasterMaterialStatus()
{  }


// creates a new material status  corresponding to this class
void
MetricStrainMasterMaterialStatus :: createSlaveStatus()
{
    StructuralMaterialStatus *status;
    status = new StructuralMaterialStatus(1, this->giveDomain(), slaveGp);
    slaveStatus =  status;
    delete status;
}


void
MetricStrainMasterMaterialStatus :: printOutputAt(FILE *file, TimeStep *tStep)
{
    //this->printSlaveMaterialOutputAt(file, tStep);      
    this->printMasterMaterialOutputAt(file, tStep);
}



void
MetricStrainMasterMaterialStatus :: printMasterMaterialOutputAt(FILE *file, TimeStep *tStep)
{
   FloatArray helpVec;
   int n;

   //fprintf(file, "  deformation gradient        ");
   fprintf(file, "  strains        ");
   StructuralMaterial :: giveFullVectorFormF( helpVec, FVector, gp->giveMaterialMode() );
   n = helpVec.giveSize();
   for ( int i = 1; i <= n; i++ ) {
     fprintf( file, " % .4e", helpVec.at(i) );
   }

   //fprintf(file, "\n              1pkstress");
   fprintf(file, "\n              stresses       ");
    StructuralMaterial :: giveFullVectorFormF( helpVec, PVector, gp->giveMaterialMode() );
    n = helpVec.giveSize();
    for ( int i = 1; i <= n; i++ ) {
        fprintf( file, " % .4e", helpVec.at(i) );
    }
    fprintf(file, "\n");
}

void 
MetricStrainMasterMaterialStatus :: printSlaveMaterialOutputAt(FILE *File, TimeStep *tNow)
// Prints the strains and stresses on the data file.
{

  FloatArray helpVec, strain,stress;
  //    int n;
    Material *mat;
    StructuralMaterial *sMat;
    mat = domain->giveMaterial(slaveMat);
    sMat = static_cast< StructuralMaterial * >(mat);
    StructuralMaterialStatus *mS = static_cast< StructuralMaterialStatus* >(sMat->giveStatus(slaveGp));    
    /*
    if(m!=0)
      fprintf(File,"              ******************************** Stress, strain, and internal variables in the %d-Seth-Hill Strain space ********************************\n",m);
    else
      fprintf(File,"              ******************************** Stress, strain, and internal variables in the Logarithmic Strain space ********************************\n");
    */
    //    fprintf(File, "            ");
    mS->printOutputAt(File, tNow);


    

}



// initializes temporary variables based on their values at the previous equlibrium state
void MetricStrainMasterMaterialStatus :: initTempStatus()
{
    Material *mat;
    StructuralMaterial *sMat;
    mat = domain->giveMaterial(slaveMat);
    sMat = static_cast< StructuralMaterial * >(mat);
    MaterialStatus *mS = sMat->giveStatus(slaveGp);
    mS->initTempStatus();
    tempPVector      = PVector;
    tempFVector      = FVector;
    //StructuralMaterialStatus :: initTempStatus();
}


// updates internal variables when equilibrium is reached
void
MetricStrainMasterMaterialStatus :: updateYourself(TimeStep *tStep)
{
    Material *mat;
    StructuralMaterial *sMat;
    mat = domain->giveMaterial(slaveMat);
    sMat = static_cast< StructuralMaterial * >(mat);
    MaterialStatus *mS = sMat->giveStatus(slaveGp);
    mS->updateYourself(tStep);
    PVector      = tempPVector;
    FVector      = tempFVector;
    //  StructuralMaterialStatus :: updateYourself(tStep);
}


// saves full information stored in this status
// temporary variables are NOT stored
contextIOResultType
MetricStrainMasterMaterialStatus :: saveContext(DataStream *stream, ContextMode mode, void *obj)
{
  /*    contextIOResultType iores;
    Material *mat;
    StructuralMaterial *sMat;
    mat = domain->giveMaterial(slaveMat);
    sMat = dynamic_cast< StructuralMaterial * >(mat);
    MaterialStatus *mS = sMat->giveStatus(gp);
    // save parent class status
    if ( ( iores = mS->saveContext(stream, mode, obj) ) != CIO_OK ) {
        THROW_CIOERR(iores);
    }
  */
    // write raw data

    return CIO_OK;
}


contextIOResultType
MetricStrainMasterMaterialStatus :: restoreContext(DataStream *stream, ContextMode mode, void *obj)
//
// restores full information stored in stream to this Status
//
{
  /*
    contextIOResultType iores;

    // read parent class status
    if ( ( iores = StructuralMaterialStatus :: restoreContext(stream, mode, obj) ) != CIO_OK ) {
        THROW_CIOERR(iores);
    }
  */
    return CIO_OK; // return succes
}
} // end namespace oofem
