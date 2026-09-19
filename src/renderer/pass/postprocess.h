// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_PASS_POSTPROCESS_H
#define RTS_PASS_POSTPROCESS_H


#include "renderer/renderer.h"

struct R_Pass_Postprocess : R_Pass
{
    Guid pipeline_id;
};

R_PASS_DECLARE_PROCS( Postprocess );


#endif
