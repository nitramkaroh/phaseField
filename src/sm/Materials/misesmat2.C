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
 *               Copyright (C) 1993 - 2013   Borek Patza
 *
 *
 *
 *       Czech Technical University, Faculty of Civil Engineering,
 *   Department of Structural Mechanics, 166 29 Prague, Czech Republic
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "misesmat2.h"
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
REGISTER_Material(MisesMat2);

// constructor
MisesMat2 :: MisesMat2(int n, Domain *d) : StructuralMaterial(n, d)
{
    linearElasticMaterial = new IsotropicLinearElasticMaterial(n, d);
    G = 0.;
    K = 0.;
}

// destructor
MisesMat2 :: ~MisesMat2()
{
    delete linearElasticMaterial;
}

// specifies whether a given material mode is supported by this model
int
MisesMat2 :: hasMaterialModeCapability(MaterialMode mode)
{
    return mode == _3dMat || mode == _1dMat || mode == _PlaneStrain;
}

// reads the model parameters from the input file
IRResultType
MisesMat2 :: initializeFrom(InputRecord *ir)
{
   
    IRResultType result;                 // required by IR_GIVE_FIELD macro

    StructuralMaterial :: initializeFrom(ir);
    linearElasticMaterial->initializeFrom(ir); // takes care of elastic constants
    G = static_cast< IsotropicLinearElasticMaterial * >(linearElasticMaterial)->giveShearModulus();
    K = static_cast< IsotropicLinearElasticMaterial * >(linearElasticMaterial)->giveBulkModulus();
	
    sig0 = 0;
    IR_GIVE_FIELD(ir, sig0, _IFT_MisesMat2_sig0); // uniaxial yield stress

    // isotropic hardening parameters
    Hi = 0.;
    Y = 0.;
    expD = 0.;
    IR_GIVE_OPTIONAL_FIELD(ir, Hi, _IFT_MisesMat2_hi); // hardening modulus
    IR_GIVE_OPTIONAL_FIELD(ir, Y, _IFT_MisesMat2_y); // hardening modulus
    IR_GIVE_OPTIONAL_FIELD(ir, expD, _IFT_MisesMat2_expD); // hardening modulus
    // kinematic hardening parametrs - only linear now				
    Hk = 0.;
    IR_GIVE_OPTIONAL_FIELD(ir, Hk, _IFT_MisesMat2_hk); // hardening modulus
    
    return IRRT_OK;
}

// creates a new material status  corresponding to this class
MaterialStatus *
MisesMat2 :: CreateStatus(GaussPoint *gp) const
{
    MisesMat2Status *status;
    status = new MisesMat2Status(1, this->giveDomain(), gp);
    return status;
}


void 
MisesMat2 :: givePinvID(FloatMatrix &answer, MaterialMode matMode)
// this matrix is the product of the 6x6 deviatoric projection matrix ID
// and the inverse scaling matrix Pinv
{

	if (matMode == _PlaneStress) {
	    answer.resize(3, 3);
	    answer.at(1,1) = answer.at(2,2) =  2. / 3.;
	    answer.at(1,2) = answer.at(2,1) = -1. / 3.;
	    answer.at(3,3) = 0.5;
	} else if (matMode == _PlaneStrain) {
	    answer.resize(4, 4);
	    answer.at(1,1) = answer.at(2,2) = answer.at(3,3) = 2. / 3.;
	    answer.at(1,2) = answer.at(1,3) = answer.at(2,1) = answer.at(2,3) = answer.at(3,1) = answer.at(3,2) = -1. / 3.;
	    answer.at(4,4) = 0.5;
	} else if(matMode == _3dMat) {
	    answer.resize(6, 6);
	    answer.at(1,1) = answer.at(2,2) = answer.at(3,3) = 2. / 3.;
	    answer.at(1,2) = answer.at(1,3) = answer.at(2,1) = answer.at(2,3) = answer.at(3,1) = answer.at(3,2) = -1. / 3.;
	    answer.at(4,4) = answer.at(5,5) = answer.at(6,6) = 0.5;
	} else {	
	    OOFEM_ERROR("bePinvID is not defined for this material mode");
	}
}

void
MisesMat2 :: giveDeltaVector(FloatArray &answer, MaterialMode matMode)
{

	if (matMode == _PlaneStress) {
		answer.resize(3);
		answer.at(1) = answer.at(2) =  1.;		
	} else if (matMode == _PlaneStrain) {
		answer.resize(4);
		answer.at(1) = answer.at(2) = answer.at(3) = 1;		
	} else if(matMode == _3dMat) {
		answer.resize(6);
		answer.at(1) = answer.at(2) = answer.at(3) = 1;
	} else {	
		OOFEM_ERROR("bePinvID is not defined for this material mode");
	}



}



void
MisesMat2 :: computeTrialStress(StressVector &answer, GaussPoint *gp, const StrainVector &elStrainDev)
{
	FloatArray backStress;
	MisesMat2Status *status = static_cast< MisesMat2Status * >( this->giveStatus(gp) );
	status->giveBackStress(backStress);
	elStrainDev.applyDeviatoricElasticStiffness(answer, G);
	answer.subtract(backStress);
}


void
MisesMat2 :: performSSPlasticityReturn(FloatArray &answer, GaussPoint *gp, const FloatArray &totalStrain)
{
    double kappa, yieldValue, dKappa = 0.;
    FloatArray reducedStress;
    FloatArray strain, plStrain;
    MaterialMode mode = gp->giveMaterialMode();
    MisesMat2Status *status = static_cast< MisesMat2Status * >( this->giveStatus(gp) );
    StressVector fullStress(mode);
    // get the initial plastic strain and initial kappa from the status
    status->givePlasticStrain(plStrain);
    kappa = status->giveCumulativePlasticStrain();
    
    // === radial return algorithm ===
    if ( mode == _1dMat ) {
      /*  LinearElasticMaterial *lmat = this->giveLinearElasticMaterial();
	  double E = lmat->give('E', gp);
	  //trial stress
	  fullStress.at(1) = E * ( totalStrain.at(1) - plStrain.at(1) );
	  
	  double trialS = fullStress.at(1);
	  trialS = fabs(trialS);
	  ///yield function
	  yieldValue = trialS - sigmaY;
	  // === radial return algorithm ===
	  if ( yieldValue > 0 ) {
	  dKappa = yieldValue / ( H + E );
	  kappa += dKappa;
	  fullStress.at(1) = fullStress.at(1) - dKappa *E *signum( fullStress.at(1) );
	  plStrain.at(1) = plStrain.at(1) + dKappa *signum( fullStress.at(1) );
	  }
		*/
    } else if ( ( mode == _PlaneStrain ) || ( mode == _3dMat ) ) {
      // elastic predictor
      StrainVector elStrain(totalStrain, mode);
      elStrain.subtract(plStrain);        
      double elStrainVol;
      StrainVector elStrainDev(mode);        
      elStrain.computeDeviatoricVolumetricSplit(elStrainDev, elStrainVol);
      double stressVol;
      stressVol = 3. * K * elStrainVol;      
      StressVector trialStressDev(mode);
      this->computeTrialStress(trialStressDev, gp, elStrainDev);		
      // store the deviatoric and trial stress (reused by algorithmic stiffness)
      status->letTrialStressDevBe(trialStressDev);        
      // check the yield condition at the trial state
      double trialS = trialStressDev.computeStressNorm();
      double	sigmaY = sqrt(2./3.)*this->computeYieldStress(kappa);        
      yieldValue = trialS - sigmaY;
      if ( yieldValue > 0. ) {
	double oldHk = this->computeBackStressModulus(kappa);
	// non-linear equation - Newtons method
	while(true) {
	  double HkP = this->computeBackStressModulusPrime(kappa + sqrt(2./3.)*dKappa);
	  double HiP = this->computeYieldStressPrime(kappa + sqrt(2./3.)*dKappa);
	  double Hi = this->computeYieldStress(kappa + sqrt(2./3.)*dKappa);
	  double Hk = this->computeBackStressModulus(kappa + sqrt(2./3.)*dKappa);	  
	  double g = -sqrt(2./3.)*Hi + trialS - 2.*G*dKappa - sqrt(2./3.)*(Hk-oldHk);
	  double Dg = -2. * G - 2./3. * (HkP + HiP);
	  dKappa -= g/Dg;
	  if( fabs(g) < 1.e-10) {
	    break;
	  }
	}
	// update equivalent plastic strain
	kappa += sqrt(2./3.) * dKappa;
	StrainVector dPlStrain(mode);
	// the following line is equivalent to multiplication by scaling matrix P
	trialStressDev.applyDeviatoricElasticCompliance(dPlStrain, 0.5);
	// increment of plastic strain
	dPlStrain.times(dKappa / trialS);
	plStrain.add(dPlStrain);			
	// scaling of deviatoric trial stress
	trialStressDev.times(1. - 2. * G * dKappa / trialS);
	//update back stress
	FloatArray backStress, tempBackStress;
	status->giveBackStress(backStress);
	tempBackStress = trialStressDev;
	tempBackStress.times(sqrt(2./3.)*(Hk-oldHk)/ trialS);
	tempBackStress.add(backStress);		
	status->letTempBackStressBe(tempBackStress);
      }
      
      // assemble the stress from the elastically computed volumetric part
      // and scaled deviatoric part
      trialStressDev.computeDeviatoricVolumetricSum(fullStress, stressVol);		
    }
    answer = fullStress;
    // store the effective stress in status;
    // store the plastic strain and cumulative plastic strain
    status->letTempPlasticStrainBe(plStrain);	
    status->setTempCumulativePlasticStrain(kappa);
}


void
MisesMat2 :: performSSPlasticityReturn_3d(FloatArray &answer, GaussPoint *gp, const FloatArray &totalStrain)
{
    double kappa, yieldValue, dKappa = 0.;
    FloatArray reducedStress;
    FloatArray strain, plStrain;
    MaterialMode mode = _3dMat;
    MisesMat2Status *status = static_cast< MisesMat2Status * >( this->giveStatus(gp) );
    StressVector fullStress(mode);
    // get the initial plastic strain and initial kappa from the status
    status->givePlasticStrain(plStrain);
    kappa = status->giveCumulativePlasticStrain();
    
    // === radial return algorithm ===
    if ( mode == _1dMat ) {
      /*  LinearElasticMaterial *lmat = this->giveLinearElasticMaterial();
	  double E = lmat->give('E', gp);
	  //trial stress
	  fullStress.at(1) = E * ( totalStrain.at(1) - plStrain.at(1) );
	  
	  double trialS = fullStress.at(1);
	  trialS = fabs(trialS);
	  ///yield function
	  yieldValue = trialS - sigmaY;
	  // === radial return algorithm ===
	  if ( yieldValue > 0 ) {
	  dKappa = yieldValue / ( H + E );
	  kappa += dKappa;
	  fullStress.at(1) = fullStress.at(1) - dKappa *E *signum( fullStress.at(1) );
	  plStrain.at(1) = plStrain.at(1) + dKappa *signum( fullStress.at(1) );
	  }
		*/
    } else if ( ( mode == _PlaneStrain ) || ( mode == _3dMat ) ) {
      // elastic predictor
      StrainVector elStrain(totalStrain, mode);
      elStrain.subtract(plStrain);        
      double elStrainVol;
      StrainVector elStrainDev(mode);        
      elStrain.computeDeviatoricVolumetricSplit(elStrainDev, elStrainVol);
      double stressVol;
      stressVol = 3. * K * elStrainVol;      
      StressVector trialStressDev(mode);
      this->computeTrialStress(trialStressDev, gp, elStrainDev);		
      // store the deviatoric and trial stress (reused by algorithmic stiffness)
      status->letTrialStressDevBe(trialStressDev);        
      // check the yield condition at the trial state
      double trialS = trialStressDev.computeStressNorm();
      double	sigmaY = sqrt(2./3.)*this->computeYieldStress(kappa);        
      yieldValue = trialS - sigmaY;
      if ( yieldValue > 0. ) {
	double oldHk = this->computeBackStressModulus(kappa);
	// non-linear equation - Newtons method
	while(true) {
	  double HkP = this->computeBackStressModulusPrime(kappa + sqrt(2./3.)*dKappa);
	  double HiP = this->computeYieldStressPrime(kappa + sqrt(2./3.)*dKappa);
	  double Hi = this->computeYieldStress(kappa + sqrt(2./3.)*dKappa);
	  double Hk = this->computeBackStressModulus(kappa + sqrt(2./3.)*dKappa);	  
	  double g = -sqrt(2./3.)*Hi + trialS - 2.*G*dKappa - sqrt(2./3.)*(Hk-oldHk);
	  double Dg = -2. * G - 2./3. * (HkP + HiP);
	  dKappa -= g/Dg;
	  if( fabs(g) < 1.e-10) {
	    break;
	  }
	}
	// update equivalent plastic strain
	kappa += sqrt(2./3.) * dKappa;
	StrainVector dPlStrain(mode);
	// the following line is equivalent to multiplication by scaling matrix P
	trialStressDev.applyDeviatoricElasticCompliance(dPlStrain, 0.5);
	// increment of plastic strain
	dPlStrain.times(dKappa / trialS);
	plStrain.add(dPlStrain);			
	// scaling of deviatoric trial stress
	trialStressDev.times(1. - 2. * G * dKappa / trialS);
	//update back stress
	FloatArray backStress, tempBackStress;
	status->giveBackStress(backStress);
	tempBackStress = trialStressDev;
	tempBackStress.times(sqrt(2./3.)*(Hk-oldHk)/ trialS);
	tempBackStress.add(backStress);		
	status->letTempBackStressBe(tempBackStress);
      }
      
      // assemble the stress from the elastically computed volumetric part
      // and scaled deviatoric part
      trialStressDev.computeDeviatoricVolumetricSum(fullStress, stressVol);		
    }
    answer = fullStress;
    // store the effective stress in status;
    // store the plastic strain and cumulative plastic strain
    status->letTempPlasticStrainBe(plStrain);	
    status->setTempCumulativePlasticStrain(kappa);
}

  
  // returns the stress vector in 3d stress space
void
MisesMat2 :: giveRealStressVector(FloatArray &answer, GaussPoint *gp, const FloatArray &totalStrain, TimeStep *atTime)
{
    MisesMat2Status *status = static_cast< MisesMat2Status * >( this->giveStatus(gp) );
    this->performSSPlasticityReturn(answer, gp, totalStrain);
    status->letTempStrainVectorBe(totalStrain);
    status->letTempStressVectorBe(answer);

}

void
MisesMat2 :: giveRealStressVector_3d(FloatArray &answer, GaussPoint *gp, const FloatArray &totalStrain, TimeStep *atTime)
{
    MisesMat2Status *status = static_cast< MisesMat2Status * >( this->giveStatus(gp) );
    this->performSSPlasticityReturn_3d(answer, gp, totalStrain);
    status->letTempStrainVectorBe(totalStrain);
    status->letTempStressVectorBe(answer);

}

void
MisesMat2 :: give3dMaterialStiffnessMatrix(FloatMatrix &answer, MatResponseMode mode, GaussPoint *gp, TimeStep *atTime)
{
  MaterialMode matMode;
  FloatArray delta;
  FloatMatrix C1,C2,dd;
  matMode = _3dMat;
  
  this->giveDeltaVector(delta,matMode);
  dd.beDyadicProductOf(delta,delta);
  //first stiffness term
  answer = dd;
  answer.times(K);
  //second stiffness term
  this->givePinvID(C1,matMode);
  C1.times(2. * G );
  answer.add(C1);

  //  this->giveLinearElasticMaterial()->giveStiffnessMatrix(answer, mode, gp, atTime);
  if ( mode != TangentStiffness ) {
    return;
  }
  
  MisesMat2Status *status = static_cast< MisesMat2Status * >( this->giveStatus(gp) );
  double tempKappa = status->giveTempCumulativePlasticStrain();
  double kappa = status->giveCumulativePlasticStrain();
  double dKappa = sqrt(1.5)*(tempKappa - kappa);
  if ( dKappa <= 0.0 ) { // elastic loading - elastic stiffness plays the role of tangent stiffness
    return;
  }
  
 
   
  double HkP = this->computeBackStressModulusPrime(kappa + sqrt(2./3.)*dKappa);
  double HiP = this->computeYieldStressPrime(kappa + sqrt(2./3.)*dKappa);
  
  double trialS;
  FloatArray trialStressDev;
  status->giveTrialStressDev(trialStressDev);
  StressVector n(trialStressDev,matMode);
  trialS = n.computeStressNorm();
  n.times(1./trialS);
  double phi = 1.- 2.*G*dKappa/trialS;
  double phiBar = 3.*G/(3.*G + HkP + HiP) -1. + phi;
  
 
  
  this->giveDeltaVector(delta,matMode);
  dd.beDyadicProductOf(delta,delta);
  //first stiffness term
  answer = dd;
  answer.times(K);
  //second stiffness term
  this->givePinvID(C1,matMode);
  C1.times(2. * G * phi );
  answer.add(C1);
  //third stiffness term
  C2.beDyadicProductOf(n,n);
  C2.times(-2. * G* phiBar);
  answer.add(C2);
}
  
void
MisesMat2 :: giveMaterialStiffnessMatrix(FloatMatrix &answer, MatResponseMode mode, GaussPoint *gp, TimeStep *atTime)
{
  this->giveLinearElasticMaterial()->giveStiffnessMatrix(answer, mode, gp, atTime);
  if ( mode != TangentStiffness ) {
    return;
  }
  
  MisesMat2Status *status = static_cast< MisesMat2Status * >( this->giveStatus(gp) );
  double tempKappa = status->giveTempCumulativePlasticStrain();
  double kappa = status->giveCumulativePlasticStrain();
  double dKappa = sqrt(1.5)*(tempKappa - kappa);
  if ( dKappa <= 0.0 ) { // elastic loading - elastic stiffness plays the role of tangent stiffness
    return;
  }
  
  MaterialMode matMode;
  matMode = gp->giveMaterialMode();
   
  double HkP = this->computeBackStressModulusPrime(kappa + sqrt(2./3.)*dKappa);
  double HiP = this->computeYieldStressPrime(kappa + sqrt(2./3.)*dKappa);
  
  double trialS;
  FloatArray trialStressDev;
  status->giveTrialStressDev(trialStressDev);
  StressVector n(trialStressDev,gp->giveMaterialMode());
  trialS = n.computeStressNorm();
  n.times(1./trialS);
  double phi = 1.- 2.*G*dKappa/trialS;
  double phiBar = 3.*G/(3.*G + HkP + HiP) -1. + phi;
  
  FloatArray delta;
  FloatMatrix C1,C2,dd;
  
  this->giveDeltaVector(delta,matMode);
  dd.beDyadicProductOf(delta,delta);
  //first stiffness term
  answer = dd;
  answer.times(K);
  //second stiffness term
  this->givePinvID(C1,matMode);
  C1.times(2. * G * phi );
  answer.add(C1);
  //third stiffness term
  C2.beDyadicProductOf(n,n);
  C2.times(-2. * G* phiBar);
  answer.add(C2);
}

///////////////////////////////////////////Large strain rutines///////////////////////////////////////////////////////////
void
MisesMat2 :: performLSPlasticityReturn(FloatMatrix &answer, GaussPoint *gp, const FloatMatrix &bBar)
{
    double kappa, yieldValue, dKappa = 0.;
    FloatArray reducedStress;
    FloatArray strain, plStrain;
    MaterialMode mode = gp->giveMaterialMode();
    MisesMat2Status *status = static_cast< MisesMat2Status * >( this->giveStatus(gp) );
    // get the initial kappa from the status
    kappa = status->giveCumulativePlasticStrain();
    // elastic predictor      
    double trBbar = bBar.at(1,1) + bBar.at(2,2) + bBar.at(3,3);
    FloatMatrix devBbar, sTrial;
    devBbar = { { bBar.at(1,1) - trBbar/3., bBar.at(2,1), bBar.at(3,1)},{bBar.at(1,2), bBar.at(2,2) - trBbar/3., bBar.at(3,2)},{bBar.at(1,3),bBar.at(2,3),bBar.at(3,3) - trBbar/3.} };
    sTrial = devBbar;
    sTrial.times(G);
    // check the yield condition at the trial state
    double normSTrial = sqrt(sTrial.at(1,1)*sTrial.at(1,1) + sTrial.at(2,2)*sTrial.at(2,2) + sTrial.at(3,3)*sTrial.at(3,3) + 2.*sTrial.at(2,3)*sTrial.at(2,3) + 2.*sTrial.at(1,3)*sTrial.at(1,3) + 2.*sTrial.at(1,2)*sTrial.at(1,2));
    double	sigmaY = sqrt(2./3.)*this->computeYieldStress(kappa);        
    yieldValue = normSTrial - sigmaY;
    if ( yieldValue > 0. ) {
      double Gbar = G * trBbar/3.;
      double oldHk = this->computeBackStressModulus(kappa);
      // non-linear equation - Newtons method
      while(true) {
	double HkP = this->computeBackStressModulusPrime(kappa + sqrt(2./3.)*dKappa);
	double HiP = this->computeYieldStressPrime(kappa + sqrt(2./3.)*dKappa);
	double Hi = this->computeYieldStress(kappa + sqrt(2./3.)*dKappa);
	double Hk = this->computeBackStressModulus(kappa + sqrt(2./3.)*dKappa);
	double g = -sqrt(2./3.)*Hi + normSTrial - 2.*Gbar*dKappa - sqrt(2./3.)*(Hk-oldHk);
	double Dg = -2. * Gbar - 2./3. * (HkP + HiP);
	dKappa -= g/Dg;
	if( fabs(g) < 1.e-10) {
	  break;
	}
      }
      // update equivalent plastic strain
      kappa += sqrt(2./3.) * dKappa;
      // scaling of deviatoric trial stress
      sTrial.times(1. - 2. * Gbar * dKappa / normSTrial);		
    }
    answer = sTrial;
    FloatMatrix B;
    // @todo change update of intermediate configuration
    B = sTrial;
    B.times(1./G);
    B.at(1,1) = B.at(1,1) + trBbar/3.;
    B.at(2,2) = B.at(2,2) + trBbar/3.;
    B.at(3,3) = B.at(3,3) + trBbar/3.;
    status->letTempLeftCauchyGreenMatrixBe(B); 
    status->setTempCumulativePlasticStrain(kappa);
}


void
MisesMat2 :: giveCauchyStressVector_3d(FloatArray &answer,GaussPoint *gp,const FloatArray &incvF,TimeStep *atTime)
{
    MisesMat2Status *status = static_cast< MisesMat2Status * >( this->giveStatus(gp) );
    MaterialMode matMode = gp->giveMaterialMode();
    
    double j,J;
    FloatArray totalStrain, vF, vP;
    FloatMatrix f, f_bar, F, Fn, invFn, bn, trBe;
    // construct deformation gradient form Fn and its increment f
    Fn.beMatrixForm(status->giveFVector());
    F.beProductOf(f,Fn);
    // construct deviatoric part of f
    j = f.giveDeterminant();
    f_bar = f;
    f_bar.times(pow(j,-1./3.));
    // trial b_e
    status->giveLeftCauchyGreenMatrix(bn);
    FloatMatrix junk;
    junk.beProductTOf(bn,f_bar);
    trBe.beProductOf(f_bar,junk);
    FloatArray vTau;
    FloatMatrix tau;
    // compute deviatoric part of Kirchhoff stress by return mapping algorithm
    this->performLSPlasticityReturn(tau, gp, trBe);
    // compute pressure
    double p = this->computePressure(J);
    // sum of dev part and vol part of the stress
    tau.at(1,1) += p;
    tau.at(2,2) += p;
    tau.at(3,3) += p;
    // conversion to 1PK stress
    answer.beSymVectorForm(tau);    
    answer.times(1./J);
    status->letTempCVectorBe(answer);
    status->letTempFVectorBe(vF); 
}
  
void
MisesMat2 :: giveFirstPKStressVector(FloatArray &answer,GaussPoint *gp,const FloatArray &reducedvF,TimeStep *atTime)
{
    MisesMat2Status *status = static_cast< MisesMat2Status * >( this->giveStatus(gp) );
    MaterialMode matMode = gp->giveMaterialMode();
    FloatMatrix F, oldF, invOldF;
    FloatArray totalStrain, vF, vP;
    StructuralMaterial :: giveFullVectorFormF(vF, reducedvF, matMode);
    F.beMatrixForm(vF); 	 
    oldF.beMatrixForm( status->giveFVector() );
    invOldF.beInverseOf(oldF);
    //relative deformation radient
    FloatMatrix f;
    f.beProductOf(F, invOldF);
    //compute elastic predictor
    FloatMatrix oldB, tempB, reducedTempB, junk;
    f.times( pow(f.giveDeterminant(), -1. / 3.) );
    status->giveLeftCauchyGreenMatrix(oldB);
    junk.beProductOf(f,oldB);
    tempB.beProductTOf(junk, f);
    FloatArray vTau, redvTau;
    FloatMatrix tau, P, invF;
    // compute deviatoric part of Kirchhoff stress by return mapping algorithm
    this->performLSPlasticityReturn(tau, gp, tempB);
    double J = F.giveDeterminant();	
    // compute pressure
    double p = this->computePressure(J);
    // sum of dev part and vol part of the stress
    tau.at(1,1) += p;
    tau.at(2,2) += p;
    tau.at(3,3) += p;
    // conversion to 1PK stress
    invF.beInverseOf(F);
    P.beProductTOf(tau,invF);    
    vP.beVectorForm(P);
    vTau.beVectorForm(tau);    
    //store Kirchoff Stress into status as a stress, will be reused by stiffness matrix
    StructuralMaterial :: giveReducedSymVectorForm(redvTau, vTau, matMode);
    StructuralMaterial :: giveReducedVectorForm(answer, vP, matMode);
    status->letTempStressVectorBe(redvTau);
    //store stress and def. gradient in status
    status->letTempPVectorBe(answer);
    status->letTempFVectorBe(vF); 
}


void
MisesMat2 :: giveMaterialStiffnessMatrix_dCde(FloatMatrix &answer, MatResponseMode mode, GaussPoint *gp, TimeStep *atTime)
{
    MisesMat2Status *status = static_cast< MisesMat2Status * >( this->giveStatus(gp) );
    // start from the elastic stiffnes`s
    double J;
    MaterialMode matMode = gp->giveMaterialMode();
    FloatArray totalStrain, vF, vP, vN, redvN, delta;
    FloatMatrix F, oldF, invOldF, fBar, oldB, junk, Id, bBar;
    FloatMatrix devBbar, sTrial, n, invF;

    oldF.beMatrixForm( status->giveFVector() );    
    invOldF.beInverseOf(oldF);
    F.beMatrixForm( status->giveTempFVector() );	       	 
    	       	 
    //relative deformation radient
    fBar.beProductOf(F, invOldF);
    //compute elastic predictor
    fBar.times( pow(fBar.giveDeterminant(), -1. / 3.) );
    status->giveLeftCauchyGreenMatrix(oldB);
    junk.beProductOf(fBar,oldB);
    bBar.beProductTOf(junk, fBar);

    this->giveDeltaVector(delta,matMode);
    this->givePinvID(Id,matMode);
   
    J = F.giveDeterminant();
    // elastic predictor      
    double trBbar = bBar.at(1,1) + bBar.at(2,2) + bBar.at(3,3);

    devBbar = { { bBar.at(1,1) - trBbar/3., bBar.at(2,1), bBar.at(3,1)},{bBar.at(1,2), bBar.at(2,2) - trBbar/3., bBar.at(3,2)},{bBar.at(1,3),bBar.at(2,3),bBar.at(3,3) - trBbar/3.} };
    sTrial = devBbar;
    sTrial.times(G);
    // check the yield condition at the trial state
    double normSTrial = sqrt(sTrial.at(1,1)*sTrial.at(1,1) + sTrial.at(2,2)*sTrial.at(2,2) + sTrial.at(3,3)*sTrial.at(3,3) + 2.*sTrial.at(2,3)*sTrial.at(2,3) + 2.*sTrial.at(1,3)*sTrial.at(1,3) + 2.*sTrial.at(1,2)*sTrial.at(1,2));
    
    n = sTrial;
    if(normSTrial != 0)
      n.times(1./normSTrial);
    
    vN.beSymVectorForm(n);
    StructuralMaterial :: giveReducedSymVectorForm(redvN, vN, matMode);

    FloatMatrix dd;
    FloatMatrix C, C1, C2, C3, Cdev;
    
    dd.beDyadicProductOf(delta, delta);
    C1 = dd; 
    //@todo different U(J)
    C1.times(K);
    //C1.times(K*J);
    
    
    //    double p = this->computePressure(J);
    C2 = dd;
    C2.times(1./3.);
    C2.add(Id);
    //@todo different U(J)
    C2.times(-2.*K*log(J));
    // C2.times(K*(1-J*J));
    
    FloatMatrix n1, n2;
    n1.beDyadicProductOf(redvN, delta);
    n2.beDyadicProductOf(delta, redvN);
    
    C3 = n1;
    C3.add(n2);
    C3.times(-2./3.*normSTrial);
    
    Cdev = Id;
    Cdev.times(2/3.*G*trBbar);
    
    Cdev.add(C3);
    
    C = C1;
    C.add(C2);
    C.add(Cdev);
    
    
    
    if ( mode == TangentStiffness ) {   
      double kappa = status->giveCumulativePlasticStrain();
      double Gbar = 1./3.*G*trBbar;
      // increment of cumulative plastic strain as an indicator of plastic loading
      double dKappa = sqrt(3. / 2.) * ( status->giveTempCumulativePlasticStrain() - kappa );
      if(dKappa > 0.) {
	// === plastic loading ===
	// trial deviatoric stress and its norm	
	double beta0, beta1, beta2, beta3, beta4;
	if ( normSTrial == 0 ) {
	  beta1 = 0;
	} else {
	  beta1 = 2./3. * G * trBbar * dKappa / normSTrial;
	}
	double hardeningModulus =  this -> computeYieldStressPrime(kappa+sqrt(2./3.)*dKappa);
	beta0 = 1. + hardeningModulus/ 3. / Gbar;
	beta2 = 2./3.*( 1. - 1. / beta0 ) * normSTrial * dKappa / Gbar;
	beta3 = 1. / beta0 - beta1 + beta2;
	beta4 = ( 1. / beta0 - beta1 ) * normSTrial / Gbar;
	
	FloatMatrix N;
	N.beDyadicProductOf(redvN, redvN);
	N.times(-2. * Gbar * beta3);	
	Cdev.times(-beta1);	
	FloatArray devNN;
	if(matMode == _PlaneStrain) {
	  FloatMatrix nn;
	  nn.beProductOf(n, n);	
	  double volNN = 1. / 3. * ( nn.at(1, 1) + nn.at(2, 2) + nn.at(3,3) );
	  devNN = {nn.at(1, 1) - volNN, nn.at(2, 2) - volNN , nn.at(3, 3) - volNN , nn.at(1, 2)  };
	} else if (matMode == _3dMat) {
	  FloatMatrix nn;
	  nn.beProductOf(n, n);	
	  double volNN = 1. / 3. * ( nn.at(1, 1) + nn.at(2, 2) + nn.at(3,3) );
	  devNN = {nn.at(1, 1) - volNN, nn.at(2, 2) - volNN , nn.at(3, 3) - volNN , nn.at(2, 3),nn.at(1, 3),nn.at(1, 2)  };
	}
	
	FloatMatrix nonSymPart,symP;
	nonSymPart.beDyadicProductOf(redvN, devNN);
	symP.beTranspositionOf(nonSymPart);
	symP.add(nonSymPart);
	symP.times(-Gbar * beta4);
	
	C.add(Cdev);
	C.add(N);
	//C.add(symP);
	nonSymPart.times(-2.*Gbar*beta4);
	C.add(nonSymPart);

      }
    }
    
    
    FloatArray vTau = status->giveTempStressVector();  
    FloatMatrix tau;
    if(matMode == _PlaneStrain) {
      if(vTau.giveSize() != 0)
	tau = {{vTau.at(1),vTau.at(4),0},{vTau.at(4),vTau.at(2),0},{0,0,vTau.at(3)}};
      else 
	tau.resize(3,3);
    } else if (matMode == _3dMat) 
	tau = {{vTau.at(1),vTau.at(6),vTau.at(5)},{vTau.at(6),vTau.at(2),vTau.at(4)},{vTau.at(5),vTau.at(4),vTau.at(3)}};
    else
	  tau.resize(3,3);
      
    
}





void
MisesMat2 :: give3dMaterialStiffnessMatrix_dPdF(FloatMatrix &answer, MatResponseMode mode, GaussPoint *gp, TimeStep *atTime)
{
    MisesMat2Status *status = static_cast< MisesMat2Status * >( this->giveStatus(gp) );
    FloatArray vTau;     
    FloatMatrix tau;
    tau.beMatrixForm(vTau);
    //    StructuralMaterial :: giveFullSymVectorForm(answer, status->giveTempStressVector(), gp->giveMaterialMode());
    FloatMatrix F, invF;
    F.beMatrixForm( status->giveTempFVector() );
    invF.beInverseOf(F);
    //    this->give_dPdF_from_dCde(answer, a,invF, tau, matMode);  
}

  

  void MisesMat2 :: give_dPdF_from_dCde(FloatMatrix &c, FloatMatrix &answer, FloatMatrix &invF, FloatMatrix &tau, MaterialMode matMode)
{ 

  if(matMode == _PlaneStrain) {
    answer.resize(5,5);
    //////////////////////////////////////////////////////
  } else if(matMode == _3dMat) {
    answer.resize(9,9);
    
    }

   
}



double 
MisesMat2 :: computeBackStressModulus(double kappa)
{
  return Hk*kappa;
}

double 
MisesMat2 :: computeBackStressModulusPrime(double kappa)
{
  return Hk;
}


double
MisesMat2 :: computeYieldStress(double kappa)
{
  return this->sig0 + this-> Hi * kappa + this->Y*(1. - exp(-expD*kappa));
}

double 
MisesMat2 :: computeYieldStressPrime(double kappa)
{ 
  return this->Hi + Y * expD * exp(-expD*kappa); 
}


double 
MisesMat2 :: computePressure(double J)
{
  //@todo different U(J)
  return  K * log(J);	
  //return (K/2.*(J-1./J));
}

int
MisesMat2 :: giveIPValue(FloatArray &answer, GaussPoint *aGaussPoint, InternalStateType type, TimeStep *atTime)
{
    MisesMat2Status *status = static_cast< MisesMat2Status * >( this->giveStatus(aGaussPoint) );
    if ( type == IST_PlasticStrainTensor ) {
        FloatArray ep;
		status->givePlasticStrain(ep);
        ///@todo Fix this so that it doesn't just fill in zeros for plane stress:
        StructuralMaterial :: giveFullSymVectorForm(answer, ep, aGaussPoint->giveMaterialMode());
        return 1;
    } else if ( type == IST_MaxEquivalentStrainLevel ) {
        answer.resize(1);
        answer.at(1) = status->giveCumulativePlasticStrain();
        return 1;
    } else {
        return StructuralMaterial :: giveIPValue(answer, aGaussPoint, type, atTime);
    }
}







//=============================================================================

MisesMat2Status :: MisesMat2Status(int n, Domain *d, GaussPoint *g) :
    StructuralMaterialStatus(n, d, g), plasticStrain(), tempPlasticStrain(), trialStressD()
{
  kappa = tempKappa = 0.;  
  leftCauchyGreenMatrix.resize(3,3);
  leftCauchyGreenMatrix.at(1,1) = 1;
  leftCauchyGreenMatrix.at(2,2) = 1;
  leftCauchyGreenMatrix.at(3,3) = 1;

}

MisesMat2Status :: ~MisesMat2Status()
{ }

void
MisesMat2Status :: printOutputAt(FILE *file, TimeStep *tStep)
{
    //int i, n;

  StructuralMaterialStatus :: printOutputAt(file, tStep);

    fprintf(file, "              status { ");
    fprintf(file, "kappa ");
    fprintf(file, " % .4e", kappa);

    fprintf(file, "}\n");
  

    fprintf(file, "}\n");
}


// initializes temporary variables based on their values at the previous equlibrium state
void MisesMat2Status :: initTempStatus()
{
    StructuralMaterialStatus :: initTempStatus();

    if ( plasticStrain.giveSize() == 0 ) {
      //plasticStrain.resize( StructuralMaterial :: giveSizeOfVoigtSymVector( gp->giveMaterialMode() ) );
      plasticStrain.resize(6);
        plasticStrain.zero();
    }

    if ( backStress.giveSize() == 0 ) {
      backStress.resize( StructuralMaterial :: giveSizeOfVoigtSymVector( gp->giveMaterialMode() ) );
      backStress.resize(6);
      backStress.zero();
    }
    tempPlasticStrain = plasticStrain;
    tempKappa = kappa;
    trialStressD.resize(0); // to indicate that it is not defined yet
	
    
}


// updates internal variables when equilibrium is reached
void
MisesMat2Status :: updateYourself(TimeStep *atTime)
{
    StructuralMaterialStatus :: updateYourself(atTime);

    plasticStrain = tempPlasticStrain;
    leftCauchyGreenMatrix = tempLeftCauchyGreenMatrix;
    kappa = tempKappa;    
    backStress = tempBackStress;
    trialStressD.resize(0); // to indicate that it is not defined any more
    
}


// saves full information stored in this status
// temporary variables are NOT stored
contextIOResultType
MisesMat2Status :: saveContext(DataStream *stream, ContextMode mode, void *obj)
{
  /*    contextIOResultType iores;

    // save parent class status
    if ( ( iores = StructuralMaterialStatus :: saveContext(stream, mode, &obj) ) != CIO_OK ) {
        THROW_CIOERR(iores);
    }

    // write raw data

    // write plastic strain (vector)
    if ( ( iores = plasticStrain.storeYourself(stream, mode) ) != CIO_OK ) {
        THROW_CIOERR(iores);
    }

    // write cumulative plastic strain (scalar)
    if ( !stream->write(& kappa, 1) ) {
        THROW_CIOERR(CIO_IOERR);
    }
  */
    return CIO_OK;
}



contextIOResultType
MisesMat2Status :: restoreContext(DataStream *stream, ContextMode mode, void *obj)
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

    // read plastic strain (vector)
    if ( ( iores = plasticStrain.restoreYourself(stream, mode) ) != CIO_OK ) {
        THROW_CIOERR(iores);
    }

    // read cumulative plastic strain (scalar)
    if ( !stream->read(& kappa, 1) ) {
        THROW_CIOERR(CIO_IOERR);
    }
  */
    return CIO_OK; // return succes
}
} // end namespace oofem
