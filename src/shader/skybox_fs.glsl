R"(

in v3 fUV;

out v4 result;

uniform samplerCube skybox;

void main()
{
    result = texture(skybox, fUV);
    //result = v4(1,1,1,1);
}

)";
