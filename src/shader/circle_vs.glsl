R"(

layout (location = 0) in v3 vP;

out smooth v3 fP;
out flat v3 center;

uniform m4x4 model;
uniform m4x4 view_proj;


void main()
{
    v4 h = model * v4(vP, 1);
    fP = h.xyz / h.w;

    center = model[3].xyz;

    gl_Position = view_proj * v4(fP, 1);
}

)";
