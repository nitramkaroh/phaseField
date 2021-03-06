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

#include "../sm/Elements/3D/qtrspacegraddamage.h"
#include "../sm/Materials/structuralms.h"
#include "../sm/CrossSections/structuralcrosssection.h"
#include "fei3dtetlin.h"
#include "node.h"
#include "material.h"
#include "gausspoint.h"
#include "gaussintegrationrule.h"
#include "floatmatrix.h"
#include "floatarray.h"
#include "intarray.h"
#include "domain.h"
#include "cltypes.h"
#include "mathfem.h"
#include "classfactory.h"

#include <cstdio>

namespace oofem {
REGISTER_Element(QTRSpaceGradDamage);

FEI3dTetLin QTRSpaceGradDamage :: interpolation_lin;

QTRSpaceGradDamage :: QTRSpaceGradDamage(int n, Domain *aDomain) :  QTRSpace(n, aDomain), GradientDamageElement()
    // Constructor.
{
    nPrimNodes = 10;
    nPrimVars = 3;
    nSecNodes = 4;
    nSecVars = 1;
    totalSize = nPrimVars * nPrimNodes + nSecVars * nSecNodes;
    locSize   = nPrimVars * nPrimNodes;
    nlSize    = nSecVars * nSecNodes;
}


IRResultType
QTRSpaceGradDamage :: initializeFrom(InputRecord *ir)
{
    numberOfGaussPoints = 4;
    return Structural3DElement :: initializeFrom(ir);
}


void
QTRSpaceGradDamage :: giveDofManDofIDMask(int inode, IntArray &answer) const
{
    if ( inode <= 4 ) {
        answer = {D_u, D_v, D_w, G_0};
    } else {
        answer = {D_u, D_v, D_w};
    }
}


void
QTRSpaceGradDamage :: giveDofManDofIDMask_u(IntArray &answer) const
{
  answer = {D_u, D_v, D_w};
}


void
QTRSpaceGradDamage :: giveDofManDofIDMask_d(IntArray &answer) const
{
  /*    if ( inode <= 4 ) {
      answer = {G_0};
    } else {
      answer = {};
    }
  */
  
}
  

void
QTRSpaceGradDamage :: computeGaussPoints()
// Sets up the array containing the four Gauss points of the receiver.
{
    integrationRulesArray.resize(1);
    integrationRulesArray [ 0 ].reset( new GaussIntegrationRule(1, this, 1, 7) );
    this->giveCrossSection()->setupIntegrationPoints(* integrationRulesArray [ 0 ], numberOfGaussPoints, this);
}


void
QTRSpaceGradDamage :: computeNdMatrixAt(GaussPoint *gp, FloatArray &answer)
{
    this->interpolation_lin.evalN(answer, gp->giveNaturalCoordinates(), FEIElementGeometryWrapper(this) );
}

void
QTRSpaceGradDamage :: computeBdMatrixAt(GaussPoint *gp, FloatMatrix &answer)
{
    FloatMatrix dnx;
    this->interpolation_lin.evaldNdx( dnx, gp->giveNaturalCoordinates(), FEIElementGeometryWrapper(this) );
    answer.beTranspositionOf(dnx);
}

}
