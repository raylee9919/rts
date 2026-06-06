// Copyright Seong Woo Lee. All Rights Reserved.


#include "./gl_bump_allocator.cpp"
#include "./gl_texture.cpp"
#include "./gl_resource.cpp"


// @Todo: Remove
//
global GLuint id_table[4096];

internal GLuint64 gl_get_bindless_handle(Opengl *gl, Asset::Texture* tex)
{
    GLuint64 handle = 0;

    if (tex->size > 0) {
        auto t = GL::alloc_texture_unique(gl, tex->incremental_id, tex->layout, tex->width, tex->height, tex->data);
        handle = t->handle;

        GL::commit_texture(gl, t->id);
    }

    return handle;
}

internal GLuint64 gl_foo(Opengl *gl, Mesh *mesh, Pbr_Texture_Type type)
{
    return gl_get_bindless_handle(gl, &mesh->textures[type]);
}

internal void opengl_compile_shaders(Opengl *gl)
{
    snprintf(g_shared, array_count(g_shared), R"(
    #define MAX_BONE_PER_VERTEX %u
    #define MAX_BONE_PER_MESH   %u
    #define MAX_LIGHTS          %u
    #define CSM_COUNT           %u
    )", MAX_BONE_PER_VERTEX, MAX_BONE_PER_MESH, MAX_LIGHTS, CSM_COUNT);

    char *pbr_vs =
    #include "shader/pbr_vs.glsl"
    char *pbr_fs =
    #include "shader/pbr_fs.glsl"

    char *skybox_vs = 
    #include "shader/skybox_vs.glsl"
    char *skybox_fs = 
    #include "shader/skybox_fs.glsl"

    char *csm_vs = 
    #include "shader/csm_vs.glsl"
    char *csm_fs = 
    #include "shader/csm_fs.glsl"

    char *simple_vs = 
    #include "shader/simple_vs.glsl"
    char *simple_fs = 
    #include "shader/simple_fs.glsl"

    {
        char *post_process_vs = 
        #include "shader/post_process_vs.glsl"
        char *post_process_fs = 
        #include "shader/post_process_fs.glsl"

        gl->post_process_program.id = opengl_program_create_vf(gl, post_process_vs, post_process_fs);
    }


    gl->pbr_program.id = opengl_program_create_vf(gl, pbr_vs, pbr_fs);
    GET_UNIFORM_LOCATION(pbr_program, u_view_proj);
    {
        glCreateBuffers(1, &gl->pbr_program.ubo);
        glNamedBufferData(gl->pbr_program.ubo, 384, NULL, GL_STREAM_DRAW);  // @Robustness: Hard-coded size...?
    }




    gl->skybox_program.id = opengl_program_create_vf(gl, skybox_vs, skybox_fs);
    GET_UNIFORM_LOCATION(skybox_program, view_proj);

    gl->shadowmap_program.id = opengl_program_create_vf(gl, csm_vs, csm_fs);
    GET_UNIFORM_LOCATION(shadowmap_program, light_view_projs);

    gl->simple_program.id = opengl_program_create_vf(gl, simple_vs, simple_fs);
    GET_UNIFORM_LOCATION(simple_program, VP);
    GET_UNIFORM_LOCATION(simple_program, color);
}

internal Render_Commands* opengl_frame_begin(Opengl *gl, v2u window_dim, v2u render_dim)
{
    Render_Commands *frame = &gl->render_commands;

    frame->window_dim = window_dim;
    frame->render_dim  = render_dim;

    frame->push_buffer_size = gl->push_buffer_size;
    frame->push_buffer_base = gl->push_buffer;
    frame->push_buffer_used = 0;

    // @Temporary
    frame->skinning_matrices = gl->skinning_matrices;

    return frame;
}

internal GLuint gl_id_from_render_id(Render_Id id)
{
    return id_table[id.e[0]];
}

internal GL_Mesh_Buffer* opengl_get_mesh_buffer(Opengl* gl, Mesh* mesh)
{
    // Does this mesh's vbo/ibo exist in the list?
    GL_Mesh_Buffer* node = gl->first_mesh_buffer;
    while (node) {
        if (node->mesh == mesh) {
            break;
        }
        node = node->next;
    }

    // If not, make one and append to list.
    if (!node) {
        node = new GL_Mesh_Buffer; // @Temporary
        zero_struct(node);

        node->mesh = mesh;

        GLsizei ilen = GLsizei(mesh->num_indices * sizeof(u32));
        GLsizei vlen = GLsizei(mesh->num_vertices * sizeof(Vertex));

        GLsizei ioffset = gl->index_buffer.used;
        GLsizei voffset = gl->vertex_buffer.used;

        glNamedBufferSubData(gl->index_buffer.handle, ioffset, ilen, mesh->indices);
        glNamedBufferSubData(gl->vertex_buffer.handle, voffset, vlen, mesh->vertices);

        GL::bump_push(&gl->index_buffer,  ilen);
        GL::bump_push(&gl->vertex_buffer, vlen);

        node->vertex_offset = voffset;
        node->index_offset  = ioffset;

        sll_push_back(gl->first_mesh_buffer, gl->last_mesh_buffer, node);
    }

    return node;
}

void gl_compute_mesh_type_count_and_instance_count(Renderer *renderer, int* num_mesh_types, int* num_instances_per_mesh, int max_types)
{
    int num_types = 0;

    // @Temporary; Use ID, and I don't want to compute number of mesh types here.
    void* ptr = NULL;
    for (u32 i = 0; i < renderer->num_meshes; ++i) {
        void *mesh = renderer->meshes[i].mesh;
        if (mesh != ptr) {
            ptr = mesh;
            num_types++;
            num_instances_per_mesh[num_types - 1] = 0;
        }

        assert(num_types < max_types);

        num_instances_per_mesh[num_types - 1]++;
    }

    *num_mesh_types = num_types;
}

void gl_record_commands(Opengl *gl, Renderer *renderer, GLuint num_instances)
{
    const u32 num_cmd = renderer->num_meshes;
    assert(num_cmd < gl->max_draw_count);

    for (u32 i = 0; i < num_cmd; ++i) 
    {
        Render_Mesh* piece = renderer->meshes + i;
        Mesh* mesh = piece->mesh;
        auto mbuf = opengl_get_mesh_buffer(gl, mesh);

        GLint base_vertex = mbuf->vertex_offset / sizeof(Vertex);
        GLint first_index = mbuf->index_offset / sizeof(u32);

        auto cmd = &gl->commands[i];
        {
            cmd->count          = mesh->num_indices;
            cmd->instance_count = num_instances;
            cmd->first_index    = first_index;
            cmd->base_vertex    = base_vertex;
            cmd->base_instance  = 0;
        }
    }

    gl->num_commands = num_cmd;
}

void gl_record_draw_params(Opengl* gl, Renderer* renderer)
{
    // Record parameters into SSBO.
    for (u32 i = 0; i < renderer->num_meshes; ++i)
    {
        Render_Mesh* piece = renderer->meshes + i;
        Mesh* mesh = piece->mesh;

        auto mat = gl->materials + i;
        {
            mat->albedo    = gl_foo(gl, mesh, PBR_ALBEDO);
            mat->normal    = gl_foo(gl, mesh, PBR_NORMAL);
            mat->roughness = gl_foo(gl, mesh, PBR_ROUGHNESS);
            mat->metallic  = gl_foo(gl, mesh, PBR_METALLIC);
            mat->emission  = gl_foo(gl, mesh, PBR_EMISSION);
        }

        auto geo = gl->geometry_params + i;
        {
            geo->world_transform               = piece->world_transform;
            geo->is_skeletal                   = piece->num_joints > 0 ? 1 : 0;
            geo->uv_scale                      = piece->uv_scale;
            geo->index_to_my_skinning_matrices = piece->index_to_my_skinning_matrices;
        }
    }
}

//void gl_draw_instances(Opengl* gl, Renderer* renderer, int num_mesh_types, int* num_instances_per_mesh)
//{
//    // Instanced Draw
//    for (int t = 0, inst = 0; t < num_mesh_types; ++t) 
//    {
//        int num_instances = num_instances_per_mesh[t];
//
//        Mesh* mesh = renderer->meshes[inst].mesh;
//        auto mbuf = opengl_get_mesh_buffer(gl, mesh);
//
//        GLint base_vertex = mbuf->vertex_offset / sizeof(Vertex);
//
//        glDrawElementsInstancedBaseVertexBaseInstance(GL_TRIANGLES,
//                                                      mesh->num_indices,
//                                                      GL_UNSIGNED_INT,
//                                                      (void*)mbuf->index_offset,
//                                                      num_instances,
//                                                      base_vertex, 
//                                                      inst);
//
//        inst += num_instances;
//    }
//}

internal void gl_frame_end(Opengl *gl, Renderer *renderer, Render_Commands *frame)
{
    u32 window_width  = frame->window_dim.w;
    u32 window_height = frame->window_dim.h;
    u32 render_width  = frame->render_dim.w;
    u32 render_height = frame->render_dim.h;

    if (gl->last_draw_width != render_width || gl->last_draw_height != render_height) {
        // Recreate color buffer.
        {
            glDeleteTextures(1, &gl->color_texture);
            glCreateTextures(GL_TEXTURE_2D, 1, &gl->color_texture);

            GLuint tex = gl->color_texture;
            glTextureStorage2D(tex, 1, GL_RGBA16F, render_width, render_height);
            glTextureParameteri(tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTextureParameteri(tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }

        // Recreate color buffer.
        {
            glDeleteTextures(1, &gl->depth_texture);
            glCreateTextures(GL_TEXTURE_2D, 1, &gl->depth_texture);

            GLuint tex = gl->depth_texture;
            glTextureStorage2D(tex, 1, GL_DEPTH_COMPONENT32F, render_width, render_height);
            glTextureParameteri(tex, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTextureParameteri(tex, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        }

        // Attach
        glNamedFramebufferTexture(gl->fbo, GL_DEPTH_ATTACHMENT, gl->depth_texture, 0);
        glNamedFramebufferTexture(gl->fbo, GL_COLOR_ATTACHMENT0, gl->color_texture, 0);

        // Check health.
        assert(glCheckNamedFramebufferStatus(gl->fbo, GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

        // Update draw size.
        gl->last_draw_width  = render_width;
        gl->last_draw_height = render_height;
    }


    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glViewport(0, 0, window_width, window_height);

        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);

        // DSA way to clear framebufers.
        float clear_color[4] = {0,0,0,0};
        float clear_depth[1] = {1};
        glClearNamedFramebufferfv(0, GL_COLOR, 0, clear_color);
        glClearNamedFramebufferfv(0, GL_DEPTH, 0, clear_depth);

        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);
    }

    // CSM Frustum
    //
    v3 *csm_frustum_positions = frame->csm_frustum_positions;
    f32 distance_between_near_and_far = distance(0.5f * (csm_frustum_positions[0] + csm_frustum_positions[3]),
                                                 0.5f * (csm_frustum_positions[4] + csm_frustum_positions[7]));
    const u32 frustum_point_count = 8 + (CSM_COUNT - 1) * 4;
    v3 frustum_positions[frustum_point_count] = {};
    {
        frustum_positions[0] = csm_frustum_positions[0];
        frustum_positions[3] = csm_frustum_positions[3];
        frustum_positions[1] = csm_frustum_positions[1];
        frustum_positions[2] = csm_frustum_positions[2];
        frustum_positions[frustum_point_count - 4] = csm_frustum_positions[4];
        frustum_positions[frustum_point_count - 1] = csm_frustum_positions[7];
        frustum_positions[frustum_point_count - 3] = csm_frustum_positions[5];
        frustum_positions[frustum_point_count - 2] = csm_frustum_positions[6];
    }

    // If you change CSM_COUNT, you need to add interpolation value in here!
    const f32 frustum_z_weights[CSM_COUNT] = {
        0.25f, 0.50f, 0.75f, 1.0f
    };
    f32 csm_z_spans[CSM_COUNT];
    for (u32 i = 0; i < CSM_COUNT; ++i) {
        csm_z_spans[i] = distance_between_near_and_far * frustum_z_weights[i];
    }

    for (u32 div = 0; div < CSM_COUNT - 1; ++div) {
        for (u32 i = 0; i < 4; ++i) {
            frustum_positions[4 + div*4 + i] = lerp(frustum_positions[i], frustum_z_weights[div], frustum_positions[frustum_point_count - (4 - i)]);
        }
    }

    u32 csm_frustum_offset[] = {
        0,1,2,2,1,3,
        4,0,6,6,0,2,
        6,2,7,7,2,3,
        1,5,3,3,5,7,
        0,4,1,1,4,5
    };
    u32 csm_outline_offset[] = {
        0,1,1,3,3,2,2,0,
        4,0,0,2,2,6,6,4,
        6,2,2,3,3,7,7,6,
        1,3,3,7,7,5,5,1,
        4,0,0,1,1,5,5,4,
    };
    const u32 csm_frustum_index_count = array_count(csm_frustum_offset);
    u32 csm_frustum_indices[csm_frustum_index_count * (CSM_COUNT + 1)];
    for (u32 level = 0; level < CSM_COUNT + 1; ++level) {
        for (u32 i = 0; i < csm_frustum_index_count; ++i) {
            csm_frustum_indices[level * csm_frustum_index_count + i] = 4 * level + csm_frustum_offset[i];
        }
    }
    const u32 csm_outline_index_count = array_count(csm_outline_offset);
    u32 csm_outline_indices[csm_outline_index_count * CSM_COUNT];
    for (u32 level = 0; level < CSM_COUNT; ++level) {
        for (u32 i = 0; i < csm_outline_index_count; ++i) {
            csm_outline_indices[level * csm_outline_index_count + i] = 4 * level + csm_outline_offset[i];
        }
    }

    m4x4 light_view_projs[CSM_COUNT];
    for (u32 level = 0; level < CSM_COUNT; ++level) {
        if (frame->csm_varient_method) {
            v3 center = {};
            for (u32 i = 0; i < 8; ++i) {
                center += frustum_positions[level*4 + i];
            }
            center *= 0.125f;
            m4x4 light_view = lookat(center + frame->csm_to_light, center, v3{0,1,0}); // TODO: Fit z?

            v3 min = v3(F32_MAX);
            v3 max = v3(-F32_MAX);
            for (u32 i = 0; i < 8; ++i) {
                v3 lp = (light_view * V4(frustum_positions[level*4 + i], 1)).xyz;
                min.x = min(min.x, lp.x);
                min.y = min(min.y, lp.y);
                min.z = min(min.z, lp.z);
                max.x = max(max.x, lp.x);
                max.y = max(max.y, lp.y);
                max.z = max(max.z, lp.z);
            }

            f32 depth = max.z - min.z; // @Todo: Fit z?

            m4x4 light_proj = ortho(min.x, max.x, min.y, max.y, -depth*2.f, depth*2.f);
            light_view_projs[level] = light_proj * light_view;
        } else {
            v3 A = frustum_positions[level*4 + 0];
            v3 B = frustum_positions[level*4 + 3];
            v3 C = frustum_positions[level*4 + 4];
            v3 D = frustum_positions[level*4 + 7];

            f32 a = distance(A, B);
            f32 b = distance(C, D);
            v3 AB = (A+B)*0.5f;
            v3 CD = (C+D)*0.5f;
            f32 h = distance(AB, CD);
            f32 k = (4*h*h + b*b - a*a) / (8*h);
            f32 r = sqrtf(k*k + a*a*0.25f);
            f32 t = map(k, 0, h);
            v3 c = lerp(AB, t, CD);

            m4x4 light_view = lookat(c + frame->csm_to_light * r, c, v3(0,1,0));
            m4x4 light_proj = ortho(-r, r, -r, r, -2*r, 2*2*r); // TODO: Constant min and max depths
            light_view_projs[level] = light_proj * light_view;
        }
    }


    // 
    int num_mesh_types = 0;
    int num_instances_per_mesh[512];
    gl_compute_mesh_type_count_and_instance_count(renderer, &num_mesh_types, num_instances_per_mesh, array_count(num_instances_per_mesh));

    // Materials, matrices...
    gl_record_draw_params(gl, renderer);

    gl_record_commands(gl, renderer, CSM_COUNT);



    // Shadow map
    //
    {
#if 1
        glViewport(0, 0, SHADOWMAP_RESOLUTION, SHADOWMAP_RESOLUTION);

        glBindFramebuffer(GL_FRAMEBUFFER, gl->shadowmap_fbo);
        defer(glBindFramebuffer(GL_FRAMEBUFFER, 0));

        float clear_depth[1] = { 1.f };
        glClearNamedFramebufferfv(gl->shadowmap_fbo, GL_DEPTH, 0, clear_depth);

        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);


        Shadowmap_Program* shadowmap_program = &gl->shadowmap_program;
        glUseProgram(shadowmap_program->id);

        glUniformMatrix4fv(shadowmap_program->light_view_projs, CSM_COUNT, true, (GLfloat *)light_view_projs);

        GLuint vao = gl->vao;

        glVertexArrayVertexBuffer(vao, 0, gl->vertex_buffer.handle, 0, sizeof(Vertex));
        glVertexArrayElementBuffer(vao, gl->index_buffer.handle);

        glEnableVertexArrayAttrib(vao, 0);
        glEnableVertexArrayAttrib(vao, 5);
        glEnableVertexArrayAttrib(vao, 6);

        glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, offset_of(Vertex, position));
        glVertexArrayAttribIFormat(vao, 5, MAX_BONE_PER_VERTEX, GL_INT, offset_of(Vertex, node_ids));
        glVertexArrayAttribFormat(vao, 6, MAX_BONE_PER_VERTEX, GL_FLOAT, GL_FALSE, offset_of(Vertex, node_weights));

        glVertexArrayAttribBinding(vao, 0, 0);
        glVertexArrayAttribBinding(vao, 5, 0);
        glVertexArrayAttribBinding(vao, 6, 0);

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, gl->skinning_matrices_buffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 9, gl->material_buffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 10, gl->geometry_param_buffer);
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, gl->draw_command_buffer);


        glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, (const void*)0, gl->num_commands, 0);
#endif
    }

    gl_record_commands(gl, renderer, 1);




    // Framebuffer Texture
    //
    {
        glBindFramebuffer(GL_FRAMEBUFFER, gl->fbo);
        defer(glBindFramebuffer(GL_FRAMEBUFFER, 0));

        glViewport(0, 0, render_width, render_height);

        // Skybox
        //
        {
            if (!gl->skybox_texture) {
                assert(frame->skybox_textures);
                auto tex = &frame->skybox_textures[0];
                Texture_Layout layout = {};
                if (tex->bytes_per_channel == 1) {
                    if (tex->num_channels == 4) {
                        layout = TEXTURE_LAYOUT_RGBA8;
                    } else if (tex->num_channels == 3) {
                        layout = TEXTURE_LAYOUT_RGB8;
                    } else {
                        INVALID_CODE_PATH;
                    }
                } else {
                    INVALID_CODE_PATH;
                }
                gl->skybox_texture = gl_create_cubemap_texture(layout, tex->width, tex->height, frame->skybox_textures, sizeof(frame->skybox_textures[0]), offset_of(Asset::Texture, data));
            }


            GLuint vao = gl->vao;

            glVertexArrayVertexBuffer(vao, 0, gl->vertex_buffer.handle, 0, sizeof(Vertex));
            glVertexArrayElementBuffer(vao, gl->index_buffer.handle);

            glEnableVertexArrayAttrib(vao, 0);
            glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, offset_of(Vertex, position));
            glVertexArrayAttribBinding(vao, 0, 0);


            glDisable(GL_CULL_FACE);
            defer(GL_CULL_FACE);

            glBindTextureUnit(0, gl->skybox_texture);

            Skybox_Program* skybox_program = &gl->skybox_program;
            glUseProgram(skybox_program->id);

            Mesh *mesh = frame->skybox_mesh;
            auto mbuf = opengl_get_mesh_buffer(gl, mesh);

            glUniformMatrix4fv(skybox_program->view_proj, 1, GL_TRUE, &frame->skybox_eye_view_proj.e[0][0]);

            // Draw call
            GLint base_vertex = mbuf->vertex_offset / sizeof(Vertex);
            glDrawElementsBaseVertex(GL_TRIANGLES, mesh->num_indices, GL_UNSIGNED_INT, (void *)mbuf->index_offset, base_vertex);
        }

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
        glClearDepth(1.0f);
        glClear(GL_DEPTH_BUFFER_BIT);

        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);


        // Static/Skeletal mesh shader.
        //
        {
            if (frame->wireframe_mode) {
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); 
            }
            defer(glPolygonMode(GL_FRONT_AND_BACK, GL_FILL));

            Pbr_Program* pbr_program = &gl->pbr_program;
            glUseProgram(pbr_program->id);

            // Bind UBO with data shared across all meshes.
            //
            // @Robustness: Hard-coded binding index.
            glBindBufferBase(GL_UNIFORM_BUFFER, 7, pbr_program->ubo);
            {
                glBufferSubData(GL_UNIFORM_BUFFER,  0, 16, &frame->wireframe_color);
                glBufferSubData(GL_UNIFORM_BUFFER, 16, 12, &frame->main_eye_position);
                glBufferSubData(GL_UNIFORM_BUFFER, 32, 12, &frame->csm_to_light);
                glBufferSubData(GL_UNIFORM_BUFFER, 48, 256, &light_view_projs[0]);
                glBufferSubData(GL_UNIFORM_BUFFER, 304, 64, &frame->csm_view);
                glBufferSubData(GL_UNIFORM_BUFFER, 368, 16, &csm_z_spans);
            }

            GLuint vao = gl->vao;

            glVertexArrayVertexBuffer(vao, 0, gl->vertex_buffer.handle, 0, sizeof(Vertex));
            glVertexArrayElementBuffer(vao, gl->index_buffer.handle);

            for (int i = 0; i <= 6; ++i) {
                glEnableVertexArrayAttrib(vao, i);
            }

            glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, offset_of(Vertex, position));
            glVertexArrayAttribFormat(vao, 1, 3, GL_FLOAT, GL_FALSE, offset_of(Vertex, normal));
            glVertexArrayAttribFormat(vao, 2, 2, GL_FLOAT, GL_FALSE, offset_of(Vertex, uv));
            glVertexArrayAttribFormat(vao, 3, 4, GL_UNSIGNED_BYTE, GL_TRUE, offset_of(Vertex, color));
            glVertexArrayAttribFormat(vao, 4, 4, GL_FLOAT, GL_FALSE, offset_of(Vertex, tangent));
            glVertexArrayAttribIFormat(vao, 5, MAX_BONE_PER_VERTEX, GL_INT, offset_of(Vertex, node_ids));
            glVertexArrayAttribFormat(vao, 6, MAX_BONE_PER_VERTEX, GL_FLOAT, GL_FALSE, offset_of(Vertex, node_weights));

            for (int i = 0; i <= 6; ++i) {
                glVertexArrayAttribBinding(vao, i, 0);
            }

            glUniformMatrix4fv(pbr_program->u_view_proj, 1, GL_TRUE, &frame->main_view_proj.e[0][0]);

            glBindTextureUnit(6, gl->shadowmaps);

            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, gl->skinning_matrices_buffer);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 9, gl->material_buffer);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 10, gl->geometry_param_buffer);
            glBindBuffer(GL_DRAW_INDIRECT_BUFFER, gl->draw_command_buffer);

            glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, (const void*)0, gl->num_commands, 0);

            //gl_draw_instances(gl, renderer, num_mesh_types, num_instances_per_mesh);
        }

        // Triangles
        //

        {
#if 0
            Simple_Program *simple_program = &gl->simple_program;
            glUseProgram(simple_program->id);

            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

            for (u8 *buffer_at = frame->push_buffer_base;
                 buffer_at < frame->push_buffer_base + frame->push_buffer_used;)
            {
                Render_Group *group = (Render_Group *)buffer_at;
                buffer_at += sizeof(Render_Group);

                for (u8 *group_at = buffer_at;
                     group_at < group->base + group->used;)
                {
                    Render_Entity_Header *entity = (Render_Entity_Header *)group_at;
                    group_at += entity->size;
                    switch (entity->type) {
                        case eRender_Triangles: {
                            Render_Triangles *piece = (Render_Triangles *)entity;

                            glEnableVertexAttribArray(0);

                            glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(Vertex), (GLvoid *)(offset_of(Vertex, position)));
                            glBufferData(GL_ARRAY_BUFFER, piece->vertexcount * sizeof(Vertex), piece->vertices, GL_DYNAMIC_DRAW);

                            glUniformMatrix4fv(simple_program->VP, 1, GL_TRUE, &frame->main_view_proj.e[0][0]);
                            glUniform4fv(simple_program->color, 1, (GLfloat *)&piece->color);

                            glBufferData(GL_ELEMENT_ARRAY_BUFFER, piece->numtri*3*sizeof(u32), piece->indices, GL_DYNAMIC_DRAW);

                            glDrawElements(GL_TRIANGLES, piece->numtri*3, GL_UNSIGNED_INT, 0);

                            glDisableVertexAttribArray(0);
                        } break;

                        default: { 
                        } break;
                    }
                }
                buffer_at += group->capacity;
            }


            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif
        }

        // Lines
        //
        {
#if 0
            Simple_Program *simple_program = &gl->simple_program;
            glUseProgram(simple_program->id);

            glDisable(GL_DEPTH_TEST);

            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

            for (u8 *buffer_at = frame->push_buffer_base;
                 buffer_at < frame->push_buffer_base + frame->push_buffer_used;)
            {
                Render_Group *group = (Render_Group *)buffer_at;
                buffer_at += sizeof(Render_Group);

                for (u8 *group_at = buffer_at;
                     group_at < group->base + group->used;)
                {
                    Render_Entity_Header *entity = (Render_Entity_Header *)group_at;
                    group_at += entity->size;
                    switch (entity->type) {
                        case eRender_Line: {
                            Render_Line *piece = (Render_Line *)entity;

                            glEnableVertexAttribArray(0);

                            glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(v3), 0);
                            glBufferData(GL_ARRAY_BUFFER, 2 * sizeof(v3), piece->p, GL_DYNAMIC_DRAW);

                            glUniformMatrix4fv(simple_program->VP, 1, GL_TRUE, &frame->main_view_proj.e[0][0]);
                            glUniform4fv(simple_program->color, 1, (GLfloat *)&piece->color);

                            u32 indices[2] = {0,1};
                            glBufferData(GL_ELEMENT_ARRAY_BUFFER, 2*sizeof(u32), indices, GL_DYNAMIC_DRAW);

                            glDrawElements(GL_LINES, 2, GL_UNSIGNED_INT, 0);

                            glDisableVertexAttribArray(0);
                        } break;

                        default: { 
                        } break;
                    }
                }
                buffer_at += group->capacity;
            }

            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glEnable(GL_DEPTH_TEST);
#endif
        }
    }


    // Post-process
    //
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glViewport(0, 0, window_width, window_height);

        auto* p = &gl->post_process_program;
        glUseProgram(p->id);

        glBindTexture(GL_TEXTURE_2D, gl->color_texture);

        glDrawArrays(GL_TRIANGLES, 0, 3);
    }


    // CSM Frustum
    //
    if (frame->draw_csm_frustum) 
    {
#if 0
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glUseProgram(gl->simple_program.id);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(v3), (GLvoid *)0);

        v4 colors[] = {
            v4{1.f,0.f,1.f,0.1f},
            v4{0.f,1.f,0.f,0.1f},
            v4{0.f,0.f,1.f,0.1f},
        };
        if (frame->draw_csm_sphere) 
        {
            for (u32 i = 0; i < array_count(colors); ++i) 
            { colors[i].a = 0.01f; }
        }

        glUniformMatrix4fv(gl->simple_program.VP, 1, true, &frame->main_view_proj.e[0][0]);
        glBufferData(GL_ARRAY_BUFFER, array_count(frustum_positions) * sizeof(*frustum_positions), frustum_positions, GL_DYNAMIC_DRAW);
        for (u32 i = 0; i < CSM_COUNT; ++i) 
        {
            glUniform4fv(gl->simple_program.color, 1, (GLfloat *)&colors[i % array_count(colors)]);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, csm_frustum_index_count * sizeof(u32), csm_frustum_indices + csm_frustum_index_count*i, GL_DYNAMIC_DRAW);
            glDrawElements(GL_TRIANGLES, csm_frustum_index_count, GL_UNSIGNED_INT, (void *)0);
        }

        v4 solid_black = V4(v3(0.f), 1.f);
        glUniform4fv(gl->simple_program.color, 1, (GLfloat *)&solid_black);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, array_count(csm_outline_indices) * sizeof(u32), csm_outline_indices, GL_DYNAMIC_DRAW);
        glDrawElements(GL_LINES, array_count(csm_outline_indices), GL_UNSIGNED_INT, (void *)0);

        glDisableVertexAttribArray(0);
        glEnable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);
#endif
    }



    //
    // Process render commands
    //
    for (u32 i = 0; i < renderer->command_count; ++i)
    {
        Render_Command cmd = renderer->commands[i];

        switch(cmd.type)
        {
            case RENDER_COMMAND_TYPE_TEXTURE_CREATE: {
                gl_alloc_texture2(gl, cmd, id_table);
            } break;

            case RENDER_COMMAND_TYPE_TEXTURE_DESTROY: {
                Render_Id id = cmd.id;
                GLuint gl_id = gl_id_from_render_id(id);

                glDeleteTextures(1, &gl_id);
            } break;

            case RENDER_COMMAND_TYPE_TEXTURE_UPDATE: {
                GLuint name = gl_id_from_render_id(cmd.texture.id);
                glTextureSubImage2D(name, 0, 0, 0, cmd.texture.width, cmd.texture.height, GL_RGBA, GL_UNSIGNED_BYTE, cmd.texture.data);
            } break;

            INVALID_DEFAULT_CASE;
        }
    }


    // @Todo: DSA
    // @Todo: DSA
    // @Todo: DSA
    // @Todo: DSA
    for (u32 type = 0; type < RENDER_VERTEX_TYPE_COUNT; type += 1)
    {
#if 1
        Render_Buffer *buffer = renderer->buffer + type;
        u64 vertex_count = buffer->vertex_count;
        u64 instance_count = buffer->instance_count;

        switch (type)
        {
            case RENDER_VERTEX_TYPE_QUAD: 
            {
                opengl_program_scope(gl->quad_program, GL_PROGRAM_CULL_OFF)
                {
                    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

                    glBindBuffer(GL_ARRAY_BUFFER, gl->vbo);
                    {
                        glBufferData(GL_ARRAY_BUFFER, sizeof(buffer->vertices[0])*vertex_count, buffer->vertices, GL_STATIC_DRAW);

                        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Render_Vertex), (void *)offset_of(Render_Vertex, position));
                        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Render_Vertex), (void *)offset_of(Render_Vertex, uv));
                        glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(Render_Vertex), (void *)offset_of(Render_Vertex, color));
                        glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, sizeof(Render_Vertex), (void *)offset_of(Render_Vertex, rect_center));
                        glVertexAttribPointer(5, 2, GL_FLOAT, GL_FALSE, sizeof(Render_Vertex), (void *)offset_of(Render_Vertex, rect_half_dim));
                        glVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE, sizeof(Render_Vertex), (void *)offset_of(Render_Vertex, radius));

                        // @Divisor Begin
                        // glVertexAttribDivisor(0, 0);

                        // NOTE: We are drawing quads independent to the ratio.
                        //         We don't want glyphs to be ugly when we resize the window.
                        glUniform1f(glGetUniformLocation(gl->quad_program.id, "viewport_w"), (f32)window_width);
                        glUniform1f(glGetUniformLocation(gl->quad_program.id, "viewport_h"), (f32)window_height);

                        for (u32 i = 0; i < instance_count; i += 1)
                        {
                            // TODO: We are being lazy and binding the texture according to the vertex's texture id.
                            //         This seems like a lunacy and at some point, it should to be fixed.
                            Render_Id texture_id = buffer->vertices[4*i].texture_id;
                            if (texture_id.e[0] == 0) {
                                glBindTexture(GL_TEXTURE_2D, gl->white_texture);
                            } else { 
                                GLuint gl_id = gl_id_from_render_id(texture_id);
                                glBindTextureUnit(0, gl_id);
                            }

                            glDrawArraysInstanced(GL_TRIANGLE_STRIP, 4*i, 4, 1);
                        }

                        // @Divisor End
                        // glVertexAttribDivisor(0, 0);
                    }
                    glBindBuffer(GL_ARRAY_BUFFER, 0);
                }
            } break;

            INVALID_DEFAULT_CASE;
        }
#endif
    }
}

// Program
//
internal Gl_Program
opengl_program_vf(Opengl *gl, char *vs_src, char *fs_src)
{
    Gl_Program program = {};
    {
        program.id = opengl_program_create_vf(gl, vs_src, fs_src);
    }

    // @acquire number of uniforms.
    GLint count;
    glGetProgramiv(program.id, GL_ACTIVE_UNIFORMS, &count);
    assert((u32)count <= gl_max_uniform_count);

    // @Gathering uniforms.
    for (GLint i = 0; i < count; i += 1)
    {
        const GLsizei sz = 64;
        GLchar name[sz];
        zero_memory(name, sz*sizeof(GLchar));
        GLsizei len;
        GLint size;
        GLenum type;
        glGetActiveUniform(program.id, i, sz, &len, &size, &type, name);

        program.uniforms[i].name = utf8c((u8 *)name);
        program.uniforms[i].id   = glGetUniformLocation(program.id, name);
    }
    program.uniform_count = count;


    // @Gathering attributes.
    glGetProgramiv(program.id, GL_ACTIVE_ATTRIBUTES, &count);
    assert((u32)count <= gl_max_attrib_count);

    for (GLint i = 0; i < count; i++)
    {
        GLint size;
        GLenum type;

        const GLsizei sz = 64;
        GLchar name[sz];
        zero_memory(name, sz*sizeof(GLchar));
        GLsizei length;
        glGetActiveAttrib(program.id, (GLuint)i, sz, &length, &size, &type, name);

        program.attribs[i].location = glGetAttribLocation(program.id, name);
        program.attribs[i].type     = type;
    }
    program.attrib_count = count;

    return program;
}

internal void
opengl_program_begin(Gl_Program program, Gl_Program_Flags flags)
{
    glUseProgram(program.id);

    for (u32 i = 0; i < program.attrib_count; i += 1) {
        Gl_Attrib attrib = program.attribs[i];
        glEnableVertexAttribArray(attrib.location);
    }

    if (flags & GL_PROGRAM_CULL_OFF) {
        glDisable(GL_CULL_FACE);
    }
}

internal void
opengl_program_end(Gl_Program program, Gl_Program_Flags flags)
{
    for (u32 i = 0; i < program.attrib_count; i += 1)
    {
        Gl_Attrib attrib = program.attribs[i];
        glDisableVertexAttribArray(attrib.location);
    }

    glUseProgram(0);
}

//
// Program creation.
//
internal GLuint
opengl_create_compute_program(Opengl *gl, const char *csrc)
{
    GLuint program = 0;

    if (glCreateShader) 
    {
        GLuint cshader = glCreateShader(GL_COMPUTE_SHADER);
        const GLchar *cunit[] = { g_shader_header, g_shared, csrc };
        glShaderSource(cshader, array_count(cunit), (const GLchar **)cunit, 0);
        glCompileShader(cshader);

        program = glCreateProgram();
        glAttachShader(program, cshader);
        glLinkProgram(program);

        glValidateProgram(program);
        GLint linked = false;
        glGetProgramiv(program, GL_LINK_STATUS, &linked);
        if (! linked) 
        {
            GLsizei stub;

            GLchar clog[1024];
            glGetProgramInfoLog(cshader, sizeof(clog), &stub, clog);

            GLchar plog[1024];
            glGetProgramInfoLog(program, sizeof(plog), &stub, plog);

            assert(!"compile/link error.");
        }

        glDeleteShader(cshader);
    } else {
        // TODO: handling.
    }
    
    return program;
}

internal GLuint
opengl_program_create_vf(Opengl *gl, char *vsrc, char *fsrc)
{
    GLuint program = 0;

    assert(glCreateShader);

    // NOTE: Compile vertex shader.
    //
    GLuint vshader = glCreateShader(GL_VERTEX_SHADER);
    {
        const GLchar *vunit[] = { g_shader_header, g_shared, vsrc };
        glShaderSource(vshader, array_count(vunit), (const GLchar **)vunit, 0);
        glCompileShader(vshader);
    }

    // NOTE: Compile fragment shader.
    //
    GLuint fshader = glCreateShader(GL_FRAGMENT_SHADER);
    {
        const GLchar *funit[] = { g_shader_header, g_shared, fsrc };
        glShaderSource(fshader, array_count(funit), (const GLchar **)funit, 0);
        glCompileShader(fshader);
    }

    // NOTE: Create program.
    //
    program = glCreateProgram();
    glAttachShader(program, vshader);
    glAttachShader(program, fshader);
    glLinkProgram(program);

    // NOTE: Validate program.
    //
    glValidateProgram(program);
    GLint linked = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (! linked) 
    {
        GLsizei stub;

        GLchar vlog[1024];
        zero_memory(vlog, sizeof(*vlog)*array_count(vlog));
        glGetShaderInfoLog(vshader, sizeof(vlog), &stub, vlog);

        GLchar flog[1024];
        zero_memory(flog, sizeof(*flog)*array_count(flog));
        glGetShaderInfoLog(fshader, sizeof(flog), &stub, flog);

        GLchar plog[1024];
        zero_memory(plog, sizeof(*plog)*array_count(plog));
        glGetProgramInfoLog(program, sizeof(plog), &stub, plog);

        assert(! "compile/link error.");
    }

    // NOTE: Cleanup.
    //
    glDeleteShader(vshader);
    glDeleteShader(fshader);

    return program;
}

internal GLuint
opengl_create_program(Opengl *gl, const char *vsrc, const char *gsrc, const char *fsrc)
{
    GLuint program = 0;

    if (glCreateShader) 
    {
        GLuint vshader = glCreateShader(GL_VERTEX_SHADER);
        const GLchar *vunit[] = { g_shader_header, g_shared, vsrc };
        glShaderSource(vshader, array_count(vunit), (const GLchar **)vunit, 0);
        glCompileShader(vshader);

        GLuint gshader = glCreateShader(GL_GEOMETRY_SHADER);
        const GLchar *gunit[] = { g_shader_header, g_shared, gsrc };
        glShaderSource(gshader, array_count(gunit), (const GLchar **)gunit, 0);
        glCompileShader(gshader);

        GLuint fshader = glCreateShader(GL_FRAGMENT_SHADER);
        const GLchar *funit[] = { g_shader_header, g_shared, fsrc };
        glShaderSource(fshader, array_count(funit), (const GLchar **)funit, 0);
        glCompileShader(fshader);

        program = glCreateProgram();
        glAttachShader(program, vshader);
        glAttachShader(program, gshader);
        glAttachShader(program, fshader);
        glLinkProgram(program);

        glValidateProgram(program);
        GLint linked = false;
        glGetProgramiv(program, GL_LINK_STATUS, &linked);
        if (!linked) 
        {
            GLsizei stub;

            GLchar vlog[1024];
            glGetShaderInfoLog(vshader, sizeof(vlog), &stub, vlog);

            GLchar glog[1024];
            glGetShaderInfoLog(gshader, sizeof(glog), &stub, glog);

            GLchar flog[1024];
            glGetShaderInfoLog(fshader, sizeof(flog), &stub, flog);

            GLchar plog[1024];
            glGetProgramInfoLog(program, sizeof(plog), &stub, plog);

            assert(!"compile/link error.");
        }

        glDeleteShader(vshader);
        glDeleteShader(gshader);
        glDeleteShader(fshader);
    } else {
        // TODO: Error-Handling.
    }
    
    return program;
}

internal GLuint
opengl_create_tessellation_program(Opengl *gl, const char *vs, const char *tcs, const char *tes, const char *fs)
{
    GLuint program = 0;

    if (glCreateShader) 
    {
        GLuint vshader = glCreateShader(GL_VERTEX_SHADER);
        const GLchar *vunit[] = { g_shader_header, g_shared, vs };
        glShaderSource(vshader, array_count(vunit), (const GLchar **)vunit, 0);
        glCompileShader(vshader);

        GLuint tcshader = glCreateShader(GL_TESS_CONTROL_SHADER);
        const GLchar *tcsunit[] = { g_shader_header, g_shared, tcs };
        glShaderSource(tcshader, array_count(tcsunit), (const GLchar **)tcsunit, 0);
        glCompileShader(tcshader);

        GLuint teshader = glCreateShader(GL_TESS_EVALUATION_SHADER);
        const GLchar *tesunit[] = { g_shader_header, g_shared, tes };
        glShaderSource(teshader, array_count(tesunit), (const GLchar **)tesunit, 0);
        glCompileShader(teshader);

        GLuint fshader = glCreateShader(GL_FRAGMENT_SHADER);
        const GLchar *funit[] = { g_shader_header, g_shared, fs };
        glShaderSource(fshader, array_count(funit), (const GLchar **)funit, 0);
        glCompileShader(fshader);

        program = glCreateProgram();
        glAttachShader(program, vshader);
        glAttachShader(program, tcshader);
        glAttachShader(program, teshader);
        glAttachShader(program, fshader);
        glLinkProgram(program);

        glValidateProgram(program);
        GLint linked = false;
        glGetProgramiv(program, GL_LINK_STATUS, &linked);
        if (!linked) 
        {
            GLsizei stub;

            GLchar vlog[1024];
            glGetShaderInfoLog(vshader, sizeof(vlog), &stub, vlog);

            GLchar tcslog[1024];
            glGetShaderInfoLog(tcshader, sizeof(tcslog), &stub, tcslog);

            GLchar teslog[1024];
            glGetShaderInfoLog(teshader, sizeof(teslog), &stub, teslog);

            GLchar flog[1024];
            glGetShaderInfoLog(fshader, sizeof(flog), &stub, flog);

            GLchar plog[1024];
            glGetProgramInfoLog(program, sizeof(plog), &stub, plog);

            assert(!"compile/link error.");
        }

        glDeleteShader(vshader);
        glDeleteShader(tcshader);
        glDeleteShader(teshader);
        glDeleteShader(fshader);
    } else {
        // TODO: Error-Handling.
    }
    
    return program;
}

internal GLuint
opengl_create_tessellation_geometry_program(Opengl *gl, const char *vs, const char *tcs, const char *tes, const char *gs, const char *fs)
{
    GLuint program = 0;

    if (glCreateShader) 
    {
        GLuint vshader = glCreateShader(GL_VERTEX_SHADER);
        const GLchar *vunit[] = { g_shader_header, g_shared, vs };
        glShaderSource(vshader, array_count(vunit), (const GLchar **)vunit, 0);
        glCompileShader(vshader);

        GLuint tcshader = glCreateShader(GL_TESS_CONTROL_SHADER);
        const GLchar *tcsunit[] = { g_shader_header, g_shared, tcs };
        glShaderSource(tcshader, array_count(tcsunit), (const GLchar **)tcsunit, 0);
        glCompileShader(tcshader);

        GLuint teshader = glCreateShader(GL_TESS_EVALUATION_SHADER);
        const GLchar *tesunit[] = { g_shader_header, g_shared, tes };
        glShaderSource(teshader, array_count(tesunit), (const GLchar **)tesunit, 0);
        glCompileShader(teshader);

        GLuint fshader = glCreateShader(GL_FRAGMENT_SHADER);
        const GLchar *funit[] = { g_shader_header, g_shared, fs };
        glShaderSource(fshader, array_count(funit), (const GLchar **)funit, 0);
        glCompileShader(fshader);

        GLuint gshader = glCreateShader(GL_GEOMETRY_SHADER);
        const GLchar *gunit[] = { g_shader_header, g_shared, gs };
        glShaderSource(gshader, array_count(gunit), (const GLchar **)gunit, 0);
        glCompileShader(gshader);

        program = glCreateProgram();
        glAttachShader(program, vshader);
        glAttachShader(program, tcshader);
        glAttachShader(program, teshader);
        glAttachShader(program, fshader);
        glAttachShader(program, gshader);
        glLinkProgram(program);

        glValidateProgram(program);
        GLint linked = false;
        glGetProgramiv(program, GL_LINK_STATUS, &linked);
        if (!linked) 
        {
            GLsizei stub;

            GLchar vlog[1024];
            glGetShaderInfoLog(vshader, sizeof(vlog), &stub, vlog);

            GLchar tcslog[1024];
            glGetShaderInfoLog(tcshader, sizeof(tcslog), &stub, tcslog);

            GLchar teslog[1024];
            glGetShaderInfoLog(teshader, sizeof(teslog), &stub, teslog);

            GLchar gslog[1024];
            glGetShaderInfoLog(gshader, sizeof(gslog), &stub, gslog);

            GLchar flog[1024];
            glGetShaderInfoLog(fshader, sizeof(flog), &stub, flog);

            GLchar plog[1024];
            glGetProgramInfoLog(program, sizeof(plog), &stub, plog);

            assert(!"compile/link error.");
        }

        glDeleteShader(vshader);
        glDeleteShader(tcshader);
        glDeleteShader(teshader);
        glDeleteShader(gshader);
        glDeleteShader(fshader);
    } else {
        // @Todo: Error-handling.
    }
    
    return program;
}


// Inits
//
internal void opengl_debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, GLchar const* message, void const* userParam)
{
#if 1
    const char *src_str = NULL;
    switch (source) {
        case GL_DEBUG_SOURCE_API: src_str = "API"; break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM: src_str = "WINDOW SYSTEM"; break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER: src_str = "SHADER COMPILER"; break;
        case GL_DEBUG_SOURCE_THIRD_PARTY: src_str = "THIRD PARTY"; break;
        case GL_DEBUG_SOURCE_APPLICATION: src_str = "APPLICATION"; break;
        case GL_DEBUG_SOURCE_OTHER: src_str = "OTHER"; break;
    }

    const char *type_str = NULL;
    switch (type) {
        case GL_DEBUG_TYPE_ERROR: type_str = "ERROR"; break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: type_str = "DEPRECATED_BEHAVIOR"; break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: type_str = "UNDEFINED_BEHAVIOR"; break;
        case GL_DEBUG_TYPE_PORTABILITY: type_str = "PORTABILITY"; break;
        case GL_DEBUG_TYPE_PERFORMANCE: type_str = "PERFORMANCE"; break;
        case GL_DEBUG_TYPE_MARKER: type_str = "MARKER"; break;
        case GL_DEBUG_TYPE_OTHER: type_str = "OTHER"; break;
    }

    const char *severity_str = NULL;
    switch (severity) {
        case GL_DEBUG_SEVERITY_NOTIFICATION: severity_str = "NOTIFICATION"; break;
        case GL_DEBUG_SEVERITY_LOW: severity_str = "LOW"; break;
        case GL_DEBUG_SEVERITY_MEDIUM: severity_str = "MEDIUM"; break;
        case GL_DEBUG_SEVERITY_HIGH: severity_str = "HIGH"; break;
    }

    printf("%s, %s, %s, %d: \n%s\n\n", src_str, type_str, severity_str, id, message);
#endif
}

internal GL_Info opengl_get_info(Opengl *gl, b32 modern_context)
{
    GL_Info info = {};
    {
        info.modern_context = modern_context;
        info.vendor         = (char *)glGetString(GL_VENDOR);
        info.renderer       = (char *)glGetString(GL_RENDERER);
        info.version        = (char *)glGetString(GL_VERSION);
    }
    
    if (info.modern_context) {
        info.shading_language_version = (char *)glGetString(GL_SHADING_LANGUAGE_VERSION);
    } else {
        info.shading_language_version = "(none)";
    }

    GLint num_extensions = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &num_extensions);

    for (GLint i = 0; i < num_extensions; ++i) {
        char *ext_name = (char *)glGetStringi(GL_EXTENSIONS, i);

        if (0) {}
        else if (string_equal(ext_name, "GL_EXT_texture_sRGB"))              { info.EXT_texture_srgb = true; }
        else if (string_equal(ext_name, "GL_EXT_framebuffer_sRGB"))          { info.ARB_EXT_framebuffer_srgb = true; }
        else if (string_equal(ext_name, "GL_ARB_framebuffer_sRGB"))          { info.ARB_EXT_framebuffer_srgb = true; }
        else if (string_equal(ext_name, "GL_ARB_framebuffer_object"))        { info.ARB_framebuffer_object = true; }
        else if (string_equal(ext_name, "GL_ARB_direct_state_access"))       { info.ARB_direct_state_access = true; }
        else if (string_equal(ext_name, "GL_ARB_explicit_uniform_location")) { info.ARB_explicit_uniform_location = true; }
        else if (string_equal(ext_name, "GL_ARB_bindless_texture"))          { info.ARB_bindless_texture = true; }
    }
    
    char *major_at = info.version;
    char *minor_at = 0;
    for (char *at = info.version; *at; ++at) 
    {
        if (at[0] == '.') 
        {
            minor_at = at + 1;
            break;
        }
    }
    
    s32 major = 1;
    s32 minor = 0;
    if (minor_at) {
        major = atoi(major_at);
        minor = atoi(minor_at);
    }
    
    
    if ((major > 2) || ((major == 2) && (minor >= 1))) {
        info.EXT_texture_srgb = true;
    }
    
    if (major >= 3) {
        info.ARB_framebuffer_object = true;
    }

    if ((major > 4) || (major == 4 && minor >= 3)) {
        info.ARB_explicit_uniform_location = true;
    }
    if ((major > 4) || (major == 4 && minor >= 5)) {
        info.ARB_direct_state_access = true;
    }


    assert(info.ARB_EXT_framebuffer_srgb);
    assert(info.ARB_direct_state_access);
    assert(info.ARB_explicit_uniform_location);
    assert(info.ARB_bindless_texture);


    // Query constants.
    //
    glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &info.max_texture_units);
    glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &info.max_attachment);
    assert(info.max_attachment >= 4);
    
    return info;
}

internal void gl_init(Opengl *gl)
{
#if 1 // @Fix: I have no idea why turning this off causes perf issue, or some other crazy problems!
    // KHR_debug has been in core since 4.3
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallbackARB(opengl_debug_callback, NULL);

    // Filter out messages.
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, NULL, GL_FALSE);
#endif

    // Textured Quad Shader
    //
    char *quad_vs = 
    #include "shader/quad_vs.glsl"
    char *quad_fs = 
    #include "shader/quad_fs.glsl"
    gl->quad_program = opengl_program_vf(gl, quad_vs, quad_fs);




    // @Todo: Clean this
    opengl_compile_shaders(gl);


    // Create default-bound textures.
    //
    gl_create_default_colored_texture(gl, 0xffffffff, gl->white, &gl->white_texture);
    gl_create_default_colored_texture(gl, 0x00000000, gl->black, &gl->black_texture);
    gl_create_default_colored_texture(gl, 0x00ff0000, gl->blue, &gl->blue_texture);



    {
        // Create shadow map texture array.
        glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &gl->shadowmaps);
        glTextureStorage3D(gl->shadowmaps, 1, GL_DEPTH_COMPONENT32F, SHADOWMAP_RESOLUTION, SHADOWMAP_RESOLUTION, CSM_COUNT);
        glTextureParameteri(gl->shadowmaps, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTextureParameteri(gl->shadowmaps, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTextureParameteri(gl->shadowmaps, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTextureParameteri(gl->shadowmaps, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        v4 border_depth[] = {1,1,1,1};
        glTextureParameterfv(gl->shadowmaps, GL_TEXTURE_BORDER_COLOR, (GLfloat *)&border_depth);
    }

    {
        // Create shadow map pass framebuffer where depths from the light's perspective will be written to.
        glCreateFramebuffers(1, &gl->shadowmap_fbo);
        glNamedFramebufferTexture(gl->shadowmap_fbo, GL_DEPTH_ATTACHMENT, gl->shadowmaps, 0);
        assert(glCheckNamedFramebufferStatus(gl->shadowmap_fbo, GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    }

    {
        glCreateFramebuffers(1, &gl->fbo);

        glGenVertexArrays(1, &gl->vao);
        glBindVertexArray(gl->vao);

        glCreateBuffers(1, &gl->vbo);

        glCreateBuffers(1, &gl->vio);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl->vio);
    }

    GL::bump_init(&gl->vertex_buffer, MB(128));
    GL::bump_init(&gl->index_buffer, MB(96));


    GLbitfield mapping_flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
    GLbitfield storage_flags = GL_DYNAMIC_STORAGE_BIT | mapping_flags;

    {
        glCreateBuffers(1, &gl->skinning_matrices_buffer);
        GLsizei sz = sizeof(m3x4) * RHI::max_num_skinning_matrices;
        glNamedBufferStorage(gl->skinning_matrices_buffer, sz, NULL, storage_flags);

        gl->skinning_matrices = (m3x4*)glMapNamedBufferRange(gl->skinning_matrices_buffer, 0, sz, mapping_flags);
    }

    {
        gl->max_draw_count = KB(16);

        glCreateBuffers(1, &gl->draw_command_buffer);
        GLsizei sz = sizeof(GL::Command) * gl->max_draw_count;
        glNamedBufferStorage(gl->draw_command_buffer, sz, NULL, storage_flags);

        gl->commands = (GL::Command*)glMapNamedBufferRange(gl->draw_command_buffer, 0, sz, mapping_flags);
    }

    {
        glCreateBuffers(1, &gl->material_buffer);
        GLsizei sz = sizeof(GL::Material) * gl->max_draw_count;
        glNamedBufferStorage(gl->material_buffer, sz, NULL, storage_flags);

        gl->materials = (GL::Material*)glMapNamedBufferRange(gl->material_buffer, 0, sz, mapping_flags);
    }

    {
        glCreateBuffers(1, &gl->geometry_param_buffer);
        GLsizei sz = sizeof(GL::Geometry_Param) * gl->max_draw_count;
        glNamedBufferStorage(gl->geometry_param_buffer, sz, NULL, storage_flags);

        gl->geometry_params = (GL::Geometry_Param*)glMapNamedBufferRange(gl->geometry_param_buffer, 0, sz, mapping_flags);
    }
}
