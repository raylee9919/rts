// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_PASS_COMPOSITION_H
#define RTS_PASS_COMPOSITION_H


#include "renderer/renderer.h"

struct R_Pass_Composition : R_Pass
{
    #include "pass/composition.h"

    Guid pipeline_id;
};

R_PASS_DECLARE_PROCS( Composition );


#endif
