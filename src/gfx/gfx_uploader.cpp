// Copyright Seong Woo Lee. All Rights Reserved.

#include "./gfx_uploader.h"

/*
  Submitting a single copy command in a list per resource sounds wasteful. 
  But at the same time, we want to issue commands asap, so the graphics queue 
  won't wait too long.

  So we'll batch the copy commands, but we don't want a batch to be too large. 
  So if a accumulated memory is big enough, we'll move on to the next batch, 
  say like 256 MB. Then, make the graphics queue wait on the semaphore of the 
  most recently issued batch.
*/
