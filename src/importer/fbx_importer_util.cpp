// Copyright Seong Woo Lee. All Rights Reserved.


v2 to_v2(ufbx_vec2 v)
{
    return v2(v.x, v.y);
}

v3 to_v3(ufbx_vec3 v)
{
    return v3(v.x, v.y, v.z);
}

v4 to_v4(ufbx_vec4 v)
{
    return v4(v.x, v.y, v.z, v.w);
}

Quaternion to_quaternion(ufbx_quat q)
{
    Quaternion quat;
    quat.w = q.w;
    quat.x = q.x;
    quat.y = q.y;
    quat.z = q.z;
    return quat;
}

m4x4 to_m4x4(ufbx_matrix umat)
{
    m4x4 m;

    // X basis
    m._11 = umat.m00;
    m._12 = umat.m01;
    m._13 = umat.m02;

    // Y basis
    m._21 = umat.m10;
    m._22 = umat.m11;
    m._23 = umat.m12;

    // Z basis
    m._31 = umat.m20;
    m._32 = umat.m21;
    m._33 = umat.m22;

    // Translation
    m._14 = umat.m03;
    m._24 = umat.m13;
    m._34 = umat.m23;

    m._41 = 0.f;
    m._42 = 0.f;
    m._43 = 0.f;
    m._44 = 1.f;

    return m;
}

const char *to_cstr(ufbx_element_type type)
{
    switch(type) {
        case UFBX_ELEMENT_UNKNOWN: return "UFBX_ELEMENT_UNKNOWN";
        case UFBX_ELEMENT_NODE: return "UFBX_ELEMENT_NODE";
        case UFBX_ELEMENT_MESH: return "UFBX_ELEMENT_MESH";
        case UFBX_ELEMENT_LIGHT: return "UFBX_ELEMENT_LIGHT";
        case UFBX_ELEMENT_CAMERA: return "UFBX_ELEMENT_CAMERA";
        case UFBX_ELEMENT_BONE: return "UFBX_ELEMENT_BONE";
        case UFBX_ELEMENT_EMPTY: return "UFBX_ELEMENT_EMPTY";
        case UFBX_ELEMENT_LINE_CURVE: return "UFBX_ELEMENT_LINE_CURVE";
        case UFBX_ELEMENT_NURBS_CURVE: return "UFBX_ELEMENT_NURBS_CURVE";
        case UFBX_ELEMENT_NURBS_SURFACE: return "UFBX_ELEMENT_NURBS_SURFACE";
        case UFBX_ELEMENT_NURBS_TRIM_SURFACE: return "UFBX_ELEMENT_NURBS_TRIM_SURFACE";
        case UFBX_ELEMENT_NURBS_TRIM_BOUNDARY: return "UFBX_ELEMENT_NURBS_TRIM_BOUNDARY";
        case UFBX_ELEMENT_PROCEDURAL_GEOMETRY: return "UFBX_ELEMENT_PROCEDURAL_GEOMETRY";
        case UFBX_ELEMENT_STEREO_CAMERA: return "UFBX_ELEMENT_STEREO_CAMERA";
        case UFBX_ELEMENT_CAMERA_SWITCHER: return "UFBX_ELEMENT_CAMERA_SWITCHER";
        case UFBX_ELEMENT_MARKER: return "UFBX_ELEMENT_MARKER";
        case UFBX_ELEMENT_LOD_GROUP: return "UFBX_ELEMENT_LOD_GROUP";
        case UFBX_ELEMENT_SKIN_DEFORMER: return "UFBX_ELEMENT_SKIN_DEFORMER";
        case UFBX_ELEMENT_SKIN_CLUSTER: return "UFBX_ELEMENT_SKIN_CLUSTER";
        case UFBX_ELEMENT_BLEND_DEFORMER: return "UFBX_ELEMENT_BLEND_DEFORMER";
        case UFBX_ELEMENT_BLEND_CHANNEL: return "UFBX_ELEMENT_BLEND_CHANNEL";
        case UFBX_ELEMENT_BLEND_SHAPE: return "UFBX_ELEMENT_BLEND_SHAPE";
        case UFBX_ELEMENT_CACHE_DEFORMER: return "UFBX_ELEMENT_CACHE_DEFORMER";
        case UFBX_ELEMENT_CACHE_FILE: return "UFBX_ELEMENT_CACHE_FILE";
        case UFBX_ELEMENT_MATERIAL: return "UFBX_ELEMENT_MATERIAL";
        case UFBX_ELEMENT_TEXTURE: return "UFBX_ELEMENT_TEXTURE";
        case UFBX_ELEMENT_VIDEO: return "UFBX_ELEMENT_VIDEO";
        case UFBX_ELEMENT_SHADER: return "UFBX_ELEMENT_SHADER";
        case UFBX_ELEMENT_SHADER_BINDING: return "UFBX_ELEMENT_SHADER_BINDING";
        case UFBX_ELEMENT_ANIM_STACK: return "UFBX_ELEMENT_ANIM_STACK";
        case UFBX_ELEMENT_ANIM_LAYER: return "UFBX_ELEMENT_ANIM_LAYER";
        case UFBX_ELEMENT_ANIM_VALUE: return "UFBX_ELEMENT_ANIM_VALUE";
        case UFBX_ELEMENT_ANIM_CURVE: return "UFBX_ELEMENT_ANIM_CURVE";
        case UFBX_ELEMENT_DISPLAY_LAYER: return "UFBX_ELEMENT_DISPLAY_LAYER";
        case UFBX_ELEMENT_SELECTION_SET: return "UFBX_ELEMENT_SELECTION_SET";
        case UFBX_ELEMENT_SELECTION_NODE: return "UFBX_ELEMENT_SELECTION_NODE";
        case UFBX_ELEMENT_CHARACTER: return "UFBX_ELEMENT_CHARACTER";
        case UFBX_ELEMENT_CONSTRAINT: return "UFBX_ELEMENT_CONSTRAINT";
        case UFBX_ELEMENT_AUDIO_LAYER: return "UFBX_ELEMENT_AUDIO_LAYER";
        case UFBX_ELEMENT_AUDIO_CLIP: return "UFBX_ELEMENT_AUDIO_CLIP";
        case UFBX_ELEMENT_POSE: return "UFBX_ELEMENT_POSE";
        case UFBX_ELEMENT_METADATA_OBJECT: return "UFBX_ELEMENT_METADATA_OBJECT";
        default: return "ERROR";
    }
}

void fbx_print_nodes(ufbx_node *node, int depth)
{
    ufbx_string name = node->name;
    printf("\n%*s%.*s\n", depth*2, " ", (int)name.length, name.data);
    printf("%*s%s\n", depth*2, " ", to_cstr(node->attrib_type));

#if 0
    {
        m4x4 m = to_m4x4(node->node_to_parent);
        printf("%*slocal transform\n", depth*2, " ");
        printf("%*s%.6f %.6f %.6f %.6f\n", depth*2, " ", m._11, m._12, m._13, m._14);
        printf("%*s%.6f %.6f %.6f %.6f\n", depth*2, " ", m._21, m._22, m._23, m._24);
        printf("%*s%.6f %.6f %.6f %.6f\n", depth*2, " ", m._31, m._32, m._33, m._34);
        printf("%*s%.6f %.6f %.6f %.6f\n", depth*2, " ", m._41, m._42, m._43, m._44);
    }

    {
        m4x4 m = to_m4x4(node->node_to_world);
        printf("%*sglobal transform\n", depth*2, " ");
        printf("%*s%.6f %.6f %.6f %.6f\n", depth*2, " ", m._11, m._12, m._13, m._14);
        printf("%*s%.6f %.6f %.6f %.6f\n", depth*2, " ", m._21, m._22, m._23, m._24);
        printf("%*s%.6f %.6f %.6f %.6f\n", depth*2, " ", m._31, m._32, m._33, m._34);
        printf("%*s%.6f %.6f %.6f %.6f\n", depth*2, " ", m._41, m._42, m._43, m._44);
    }
#endif

    ufbx_node_list children = node->children;
    for (int i = 0; i < (int)children.count; ++i) {
        ufbx_node *child = children.data[i];
        fbx_print_nodes(child, depth + 1);
    }
}
