// Copyright Seong Woo Lee. All Rights Reserved.

import basic;
#include "shared/shared.h"

struct Push_Constants
{
};

[shader("compute")]
[numthreads(32, 1, 1)]
void main_cs(uint3 tid : SV_DispatchThreadID) 
{
}
