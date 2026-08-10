// Copyright Seong Woo Lee. All Rights Reserved.

R"(

// A single draw call normally only renders to one render target layer at a time.
// That's why I need this geometry shader for my cascaded shadow maps.

//layout(triangles, invocations = CSM_COUNT) in;
//layout(triangle_strip, max_vertices = 3) out;
//
//uniform m4x4 light_view_projs[CSM_COUNT];
//
//void main()
//{          
//    for (u32 i = 0; i < 3; ++i) {
//        gl_Position = light_view_projs[gl_InvocationID] * gl_in[i].gl_Position;
//        gl_Layer = gl_InvocationID;
//        EmitVertex();
//    }
//    EndPrimitive();
//}  

)";
