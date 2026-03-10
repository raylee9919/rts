// Copyright Seong Woo Lee. All Rights Reserved.

v2 to_v2(ufbx_vec2 v);
v3 to_v3(ufbx_vec3 v);
v4 to_v4(ufbx_vec4 v);
m4x4 to_m4x4(ufbx_matrix umat);
Quaternion to_quaternion(ufbx_quat q);

void fbx_print_nodes(ufbx_node *node, int depth = 0);
