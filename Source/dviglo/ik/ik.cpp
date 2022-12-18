// Copyright (c) 2008-2022 the Urho3D project
// Copyright (c) 2022-2022 the Dviglo project
// License: MIT

#include "../ik/ik.h"
#include "../ik/ik_constraint.h"
#include "../ik/ik_effector.h"
#include "../ik/ik_solver.h"

namespace Urho3D
{

const char* IK_CATEGORY = "Inverse Kinematics";

// ----------------------------------------------------------------------------
void RegisterIKLibrary(Context* context)
{
    //IKConstraint::RegisterObject(context);
    IKEffector::RegisterObject(context);
    IKSolver::RegisterObject(context);
}

} // namespace Urho3D
