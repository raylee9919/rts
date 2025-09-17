/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright 2025 by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */



    
internal void
gl_voxelize_scene(s32 write, Render_Batch *batch, m4x4 voxelize_clip_P, v3 light_P, v3 light_color, f32 light_strength)
{
    for (Render_Group *group = (Render_Group *)batch->base;
         (u8 *)group < (u8 *)batch->base + batch->used;
         ++group)
    {
        for (u8 *at = group->base;
             at < group->base + group->used;
             at += ((Render_Entity_Header *)at)->size)
        {
            Render_Entity_Header *entity = (Render_Entity_Header *)at;
            switch (entity->type)
            {
                case eRender_Mesh:
                {
                    Render_Mesh *piece = (Render_Mesh *)entity;
                    Mesh *mesh            = piece->mesh;
                    Material *mat         = piece->material;

                    // Use voxelization program.
                    Voxelization_Program *program = &gl.voxelization_program;
                    s32 pid = program->id;
                    GL(glUseProgram(pid));

                    // @TEMPORARY...?
                    Camera *camera = group->camera;

                    GL(glUniformMatrix4fv(program->world_transform, 1, true, &piece->world_transform.e[0][0]));
                    GL(glUniformMatrix4fv(program->V, 1, GL_TRUE, &group->V.e[0][0]));
                    GL(glUniformMatrix4fv(program->voxel_P, 1, GL_TRUE, &voxelize_clip_P.e[0][0]));
                    GL(glUniform1i(program->is_skeletal, piece->animation_transforms ? 1 : 0));
                    GL(glUniform3fv(program->ambient, 1, (GLfloat *)&mat->color_ambient));
                    GL(glUniform3fv(program->diffuse, 1, (GLfloat *)&mat->color_diffuse));
                    GL(glUniform3fv(program->specular, 1, (GLfloat *)&mat->color_specular));

                    GL(glUniform1ui(program->octree_level, OCTREE_LEVEL));
                    GL(glUniform1ui(program->octree_resolution, gl.octree_resolution));
                    GL(glUniform1ui(program->write, write));

                    GL(glUniform3fv(program->DEBUG_light_P, 1, (GLfloat *)&light_P));
                    GL(glUniform3fv(program->DEBUG_light_color, 1, (GLfloat *)&light_color));
                    GL(glUniform1f(program->DEBUG_light_strength, light_strength));

                    GL(glEnableVertexAttribArray(0));
                    GL(glEnableVertexAttribArray(1));
                    GL(glEnableVertexAttribArray(2));
                    GL(glEnableVertexAttribArray(3));
                    GL(glEnableVertexAttribArray(4));
                    GL(glEnableVertexAttribArray(5));

                    GL(glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(Vertex), (GLvoid *)(offset_of(Vertex, pos))));
                    GL(glVertexAttribPointer(1, 3, GL_FLOAT, false, sizeof(Vertex), (GLvoid *)(offset_of(Vertex, normal))));
                    GL(glVertexAttribPointer(2, 2, GL_FLOAT, false, sizeof(Vertex), (GLvoid *)(offset_of(Vertex, uv))));
                    GL(glVertexAttribPointer(3, 4, GL_FLOAT, true,  sizeof(Vertex), (GLvoid *)(offset_of(Vertex, color))));
                    GL(glVertexAttribIPointer(4, MAX_BONE_PER_VERTEX, GL_INT, sizeof(Vertex), (GLvoid *)(offset_of(Vertex, node_ids))));
                    GL(glVertexAttribPointer(5, MAX_BONE_PER_VERTEX, GL_FLOAT, false, sizeof(Vertex), (GLvoid *)(offset_of(Vertex, node_weights))));

                    GL(glBufferData(GL_ARRAY_BUFFER,
                                 mesh->vertex_count * sizeof(Vertex),
                                 mesh->vertices,
                                 GL_DYNAMIC_DRAW));

                    GL(glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                                 mesh->index_count * sizeof(u32),
                                 mesh->indices,
                                 GL_DYNAMIC_DRAW));

                    if (piece->animation_transforms)
                        GL(glUniformMatrix4fv(program->bone_transforms, MAX_BONE_PER_MESH, true, (GLfloat *)piece->animation_transforms));

                    GL(glDrawElements(GL_TRIANGLES, mesh->index_count, GL_UNSIGNED_INT, (void *)0));
                    glMemoryBarrier(GL_ATOMIC_COUNTER_BARRIER_BIT|GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

                    GL(glDisableVertexAttribArray(0));
                    GL(glDisableVertexAttribArray(1));
                    GL(glDisableVertexAttribArray(2));
                    GL(glDisableVertexAttribArray(3));
                    GL(glDisableVertexAttribArray(4));
                    GL(glDisableVertexAttribArray(5));
                } break;

                case eRender_Grass:
                {
                } break;

                case eRender_Star:
                {
                } break;

                case eRender_Bitmap:
                {
                } break;

                INVALID_DEFAULT_CASE;
            }
        }
    }
}



        f32 a = 1.0f / VOXEL_HALF_SIDE;
        m4x4 voxelize_clip_P = m4x4{{
            { a,  0,  0,  0},
            { 0,  a,  0,  0},
            { 0,  0, -a , 0},
            { 0,  0,  0,  1}
        }};
        f32 voxel_in_meter = ((2.0f*VOXEL_HALF_SIDE) / (gl.octree_resolution));

        // Settings
        GL(glViewport(0, 0, gl.octree_resolution, gl.octree_resolution));
        GL(glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE));
        GL(glDisable(GL_CULL_FACE));
        GL(glDisable(GL_DEPTH_TEST));
        GL(glDisable(GL_BLEND));
        GL(glDisable(GL_SCISSOR_TEST));
        GL(glDisable(GL_MULTISAMPLE));

        GL(glBindBuffer(GL_ARRAY_BUFFER, gl.vbo));

        // Prepare atomic fragment-counter.
        GL(glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, gl.fragment_counter));
        GL(glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, gl.fragment_counter));

        gl_voxelize_scene(0, batch, voxelize_clip_P, light_P, light_color, light_strength);

        GL(glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, gl.fragment_counter));
        u32 *mapped_fragment_count = (u32 *)glMapBufferRange(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), GL_MAP_READ_BIT|GL_MAP_WRITE_BIT);
        u32 fragment_count = mapped_fragment_count[0];
        *mapped_fragment_count = 0;
        GL(glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER));
        GL(glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0));

        if (gl.max_fragment_count < fragment_count)
        {
            gl.max_fragment_count = fragment_count;
            gl_gen_linear_buffer(&gl.flist_P, &gl.flist_P_texture, GL_R32UI, sizeof(u32) * fragment_count);
            gl_gen_linear_buffer(&gl.flist_diffuse, &gl.flist_diffuse_texture, GL_RGBA8, sizeof(u32) * fragment_count);
        }

        GL(glBindImageTexture(0, gl.flist_P_texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGB10_A2UI));
        GL(glBindImageTexture(1, gl.flist_diffuse_texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8));

        gl_voxelize_scene(1, batch, voxelize_clip_P, light_P, light_color, light_strength);

        GL(glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, gl.fragment_counter));
        mapped_fragment_count = (u32 *)glMapBufferRange(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), GL_MAP_READ_BIT|GL_MAP_WRITE_BIT);
        u32 fragment_count2 = mapped_fragment_count[0];
        Assert(fragment_count == fragment_count2);
        *mapped_fragment_count = 0;
        GL(glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER)); // @TODO: Is unmapping necessary?
        GL(glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0));
        glMemoryBarrier(GL_ATOMIC_COUNTER_BARRIER_BIT);


        //
        // Build Octree
        //
#if 1
        // Reset variables
        s32 wx, wy, gx, gy;
        u32 one = 1;
        u32 start = 0;
        u32 level_start[OCTREE_LEVEL + 1];
        u32 level_end[OCTREE_LEVEL + 1];
        level_start[0] = 0;
        level_end[0] = 1;

        // Reset alloc counter to 1.
        GL(glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, gl.alloc_count));
        GL(glBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(u32), &one));
        GL(glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, gl.alloc_count));

        // 'Level' to be allocated.
        for (u32 level = 1;
             level <= OCTREE_LEVEL;
             ++level)
        {
            //
            // 1. Flag
            //
            Flag_Program *fp = &gl.flag_program;
            GL(glUseProgram(fp->id));

            GL(glBindImageTexture(0, gl.flist_P_texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGB10_A2UI));
            GL(glBindImageTexture(1, gl.octree_nodes_texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI));
            GL(glUniform1ui(fp->current_level, level));
            GL(glUniform1ui(fp->octree_level, OCTREE_LEVEL));
            GL(glUniform1ui(fp->octree_resolution, gl.octree_resolution));
            GL(glUniform1ui(fp->fragment_count, fragment_count));

            wx = 1024;
            wy = (fragment_count + wx - 1) / wx;
            gx = 128;
            gy = ((wy + 7) >> 3);
            Assert((gx*gy*8*8) >= (s32)fragment_count);
            Assert(gx <= gl.max_compute_work_group_count[0] &&
                   gy <= gl.max_compute_work_group_count[1]);
            GL(glDispatchCompute(gx, gy, 1));
            GL(glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT));


            //
            // 2. Alloc
            //
            Alloc_Program *ap = &gl.alloc_program;
            glUseProgram(ap->id);

            u32 prev_alloc_count;
            GL(glGetBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(u32), &prev_alloc_count));
            u32 node_count = (prev_alloc_count - (start >> 3));
            u32 alloc_size = (prev_alloc_count << 3);

            GL(glUniform1ui(ap->alloc_size, alloc_size));
            GL(glUniform1ui(ap->start, start));
            GL(glBindImageTexture(0, gl.octree_nodes_texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI));

            wx = 1024;
            wy = ((node_count << 3) + wx - 1) / wx;
            gx = 128;
            gy = ((wy + 7) >> 3);
            Assert((gx*gy*8*8) >= (s32)(node_count<<3));
            Assert(gx <= gl.max_compute_work_group_count[0] &&
                   gy <= gl.max_compute_work_group_count[1]);
            GL(glDispatchCompute(gx, gy, 1));
            GL(glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT|GL_ATOMIC_COUNTER_BARRIER_BIT));

            // Update
            start = (prev_alloc_count << 3);
            GL(glGetBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(u32), &prev_alloc_count));
            node_count = (prev_alloc_count - (start >> 3));
            alloc_size = (prev_alloc_count << 3);
            level_start[level] = start;
            level_end[level] = alloc_size;

            //
            // 3. Init
            //
            Init_Program *ip = &gl.init_program;
            GL(glUseProgram(ip->id));

            GL(glBindImageTexture(0, gl.octree_nodes_texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI));
            GL(glUniform1ui(ip->start, start));
            GL(glUniform1ui(ip->alloc_size, alloc_size));

            wx = 1024;
            wy = ((node_count << 3) + wx - 1) / wx;
            gx = 128;
            gy = ((wy + 7) >> 3);
            Assert(gx * gy * 8 * 8 >= (s32)(node_count << 3));
            Assert(gx <= gl.max_compute_work_group_count[0] &&
                   gy <= gl.max_compute_work_group_count[1]);
            GL(glDispatchCompute(gx, gy, 1));
            GL(glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT|GL_ATOMIC_COUNTER_BARRIER_BIT));
        }

        // get alloced node #.
        u32 node_count;
        GL(glGetBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(u32), &node_count));
        GL(glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0));

        // flag all leaves for mip-mapping.
        u32 work_size;
#if 0
        Flag_Leaf_Program *fp = &gl.flag_leaf_program;
        GL(glUseProgram(fp->id));

        GL(glBindImageTexture(0, gl.octree_nodes_texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI));
        GL(glUniform1ui(fp->leaf_start, level_start[OCTREE_LEVEL]));
        GL(glUniform1ui(fp->leaf_end, level_end[OCTREE_LEVEL]));

        work_size = (level_end[OCTREE_LEVEL] - level_start[OCTREE_LEVEL]);
        wx = 1024;
        wy = (work_size + wx - 1) / wx;
        gx = 128;
        gy = ((wy + 7) >> 3);
        Assert((gx*gy*8*8) >= (s32)work_size);
        Assert(gx <= gl.max_compute_work_group_count[0] &&
               gy <= gl.max_compute_work_group_count[1]);
        GL(glDispatchCompute(gx, gy, 1));
        GL(glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT));
#endif

        u32 zero = 0;
        //
        // Creating (probably) memory-saved Octree Attribute Buffer.
        //
        u32 buffer_size = (sizeof(u32) * (node_count << 3));
        gl_gen_linear_buffer(&gl.octree_diffuse, &gl.octree_diffuse_texture, GL_R32UI, buffer_size);
        GL(glClearNamedBufferData(gl.octree_diffuse, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, &zero));

        Octree_Program *op = &gl.octree_program;
        GL(glUseProgram(op->id));

        GL(glBindImageTexture(0, gl.flist_P_texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGB10_A2UI));
        GL(glBindImageTexture(1, gl.flist_diffuse_texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8));
        GL(glBindImageTexture(2, gl.octree_nodes_texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI));
        GL(glBindImageTexture(3, gl.octree_diffuse_texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI));

        GL(glUniform1ui(op->octree_level, OCTREE_LEVEL));
        GL(glUniform1ui(op->octree_resolution, gl.octree_resolution));
        GL(glUniform1ui(op->fragment_count, fragment_count));

        wx = 1024;
        wy = (fragment_count + wx - 1) / wx;
        gx = 128;
        gy = ((wy + 7) >> 3);
        Assert((gx*gy*8*8) >= (s32)fragment_count);
        Assert(gx <= gl.max_compute_work_group_count[0] &&
               gy <= gl.max_compute_work_group_count[1]);
        GL(glDispatchCompute(gx, gy, 1));
        GL(glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT));
#endif

        //
        // build octree mipmap from leaf.
        //
#if 1
        Mipmap_Program *mp = &gl.mipmap_program;
        GL(glUseProgram(mp->id));

        GL(glBindImageTexture(0, gl.octree_nodes_texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI));
        GL(glBindImageTexture(1, gl.octree_diffuse_texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI));

        for (u32 i = 1; i <= OCTREE_LEVEL; ++i)
        {
            u32 level = (OCTREE_LEVEL - i);

            GL(glUniform1ui(mp->level_start, level_start[level]));
            GL(glUniform1ui(mp->level_end, level_end[level]));

            work_size = (level_end[level] - level_start[level]);
            wx = 1024;
            wy = (work_size + wx - 1) / wx;
            gx = 128;
            gy = ((wy + 7) >> 3);
            Assert((gx*gy*8*8) >= (s32)work_size);
            Assert(gx <= gl.max_compute_work_group_count[0] &&
                   gy <= gl.max_compute_work_group_count[1]);
            GL(glDispatchCompute(gx, gy, 1));
            GL(glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT));
        }
#endif





        //
        // Constructing G-Buffer
        //
        GL(glBindFramebuffer(GL_FRAMEBUFFER, gl.gbuffer.frame_buffer));
        GL(glBindBuffer(GL_ARRAY_BUFFER, gl.vbo));

        // Settings
        GL(glViewport(0, 0, resolution.w, resolution.h));
        GL(glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE));
        GL(glDisable(GL_BLEND));
        GL(glDisable(GL_SCISSOR_TEST));
        GL(glEnable(GL_DEPTH_TEST));
        GL(glClearColor(0, 0, 0, 0));
        GL(glClear(GL_COLOR_BUFFER_BIT));
        GL(glClearDepth(1));
        GL(glClear(GL_DEPTH_BUFFER_BIT));
        GL(glDepthFunc(GL_LEQUAL));
        GL(glDisable(GL_MULTISAMPLE));
        GL(glEnable(GL_CULL_FACE));
        GL(glCullFace(GL_BACK));
        GL(glFrontFace(GL_CCW));

        //
        v3 cam_P;
        {
            G_Buffer_Program *program = &gl.gbuffer_program;
            s32 pid = gl.gbuffer_program.id;
            glUseProgram(pid);

            for (Render_Group *group = (Render_Group *)batch->base;
                 (u8 *)group < (u8 *)batch->base + batch->used;
                 ++group)
            {
                for (u8 *at = group->base;
                     at < group->base + group->used;
                     at += ((Render_Entity_Header *)at)->size)
                {
                    Render_Entity_Header *entity = (Render_Entity_Header *)at;
                    switch (entity->type)
                    {
                        case eRender_Mesh:
                            {
                                Render_Mesh *piece = (Render_Mesh *)entity;
                                Mesh *mesh         = piece->mesh;
                                Material *mat      = piece->material;

                                Camera *camera = group->camera;

                                GL(glUniformMatrix4fv(program->VP, 1, GL_TRUE, &group->VP.e[0][0]));
                                GL(glUniform1i(program->is_skeletal, piece->animation_transforms ? 1 : 0));
                                GL(glUniformMatrix4fv(program->world_transform, 1, true, &piece->world_transform.e[0][0]));
                                GL(glUniform3fv(program->ambient, 1, (GLfloat *)&mat->color_ambient));
                                GL(glUniform3fv(program->diffuse, 1, (GLfloat *)&mat->color_diffuse));
                                GL(glUniform3fv(program->specular, 1, (GLfloat *)&mat->color_specular));
                                GL(glUniform1ui(program->entity_id, piece->entity_id));

                                GL(glEnableVertexAttribArray(0));
                                GL(glEnableVertexAttribArray(1));
                                GL(glEnableVertexAttribArray(2));
                                GL(glEnableVertexAttribArray(3));
                                GL(glEnableVertexAttribArray(4));
                                GL(glEnableVertexAttribArray(5));

                                GL(glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(Vertex), (GLvoid *)(offset_of(Vertex, pos))));
                                GL(glVertexAttribPointer(1, 3, GL_FLOAT, false, sizeof(Vertex), (GLvoid *)(offset_of(Vertex, normal))));
                                GL(glVertexAttribPointer(2, 2, GL_FLOAT, false, sizeof(Vertex), (GLvoid *)(offset_of(Vertex, uv))));
                                GL(glVertexAttribPointer(3, 4, GL_FLOAT, true,  sizeof(Vertex), (GLvoid *)(offset_of(Vertex, color))));
                                GL(glVertexAttribIPointer(4, MAX_BONE_PER_VERTEX, GL_INT, sizeof(Vertex), (GLvoid *)(offset_of(Vertex, node_ids))));
                                GL(glVertexAttribPointer(5, MAX_BONE_PER_VERTEX, GL_FLOAT, false, sizeof(Vertex), (GLvoid *)(offset_of(Vertex, node_weights))));

                                GL(glBufferData(GL_ARRAY_BUFFER,
                                                mesh->vertex_count * sizeof(Vertex),
                                                mesh->vertices,
                                                GL_DYNAMIC_DRAW));

                                GL(glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                                                mesh->index_count * sizeof(u32),
                                                mesh->indices,
                                                GL_DYNAMIC_DRAW));

                                if (piece->animation_transforms)
                                    GL(glUniformMatrix4fv(program->bone_transforms, MAX_BONE_PER_MESH, true, (GLfloat *)piece->animation_transforms));

                                GL(glDrawElements(GL_TRIANGLES, mesh->index_count, GL_UNSIGNED_INT, (void *)0));
                                GL(glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT));

                                GL(glDisableVertexAttribArray(0));
                                GL(glDisableVertexAttribArray(1));
                                GL(glDisableVertexAttribArray(2));
                                GL(glDisableVertexAttribArray(3));
                                GL(glDisableVertexAttribArray(4));
                                GL(glDisableVertexAttribArray(5));
                            } break;
                    }
                }
            }

            gl_process_mouse(&result, input);
        }

        //
        // Draw
        //
#if 1
        GL(glBindFramebuffer(GL_FRAMEBUFFER, 0));
        GL(glViewport(0, 0, win.w, win.h));

        GL(glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE));

        GL(glEnable(GL_BLEND));
        GL(glBlendFunc(GL_ONE, GL_ONE_MINUS_DST_ALPHA));

        GL(glEnable(GL_SCISSOR_TEST));
        GL(glScissor(0, 0, win.w, win.h));

        GL(glEnable(GL_DEPTH_TEST));
        GL(glClear(GL_COLOR_BUFFER_BIT));
        GL(glClearDepth(1.0f));
        GL(glClear(GL_DEPTH_BUFFER_BIT));
        GL(glDepthFunc(GL_LEQUAL));

        GL(glDisable(GL_MULTISAMPLE));

        GL(glEnable(GL_CULL_FACE));
        GL(glCullFace(GL_BACK));
        GL(glFrontFace(GL_CCW));

#if 0
        int DEBUG_DRAW_MODE = VV;

        for (Render_Group *group = (Render_Group *)batch->base;
             (u8 *)group < (u8 *)batch->base + batch->used;
             ++group)
        {
            for (u8 *at = group->base;
                 at < group->base + group->used;
                 at += ((Render_Entity_Header *)at)->size)
            {
                Render_Entity_Header *entity = (Render_Entity_Header *)at;
                switch (entity->type)
                {
                    case eRender_Mesh:
                        {
                            switch (DEBUG_DRAW_MODE)
                            {
                                case VV:
                                    {
                                        Render_Mesh *piece = (Render_Mesh *)entity;
                                        Mesh *mesh            = piece->mesh;
                                        Material *mat         = piece->material;

                                        glBindBuffer(GL_ARRAY_BUFFER, gl.vbo);

                                        Voxel_Program *program = &gl.voxel_program;
                                        s32 pid = program->id;
                                        glUseProgram(pid);

                                        Camera *camera = group->camera;

                                        glUniformMatrix4fv(program->world_transform, 1, true, &piece->world_transform.e[0][0]);
                                        glUniformMatrix4fv(program->V, 1, GL_TRUE, &camera->V.e[0][0]);
                                        glUniformMatrix4fv(program->persp_P, 1, GL_TRUE, &camera->P.e[0][0]);
                                        glUniformMatrix4fv(program->ortho_P, 1, GL_TRUE, &voxelize_clip_P.e[0][0]);
                                        glUniform1i(program->is_skeletal, piece->animation_transforms ? 1 : 0);
                                        if (piece->animation_transforms)
                                            glUniformMatrix4fv(program->bone_transforms, MAX_BONE_PER_MESH, true, (GLfloat *)piece->animation_transforms);
                                        glUniform1ui(program->octree_level, OCTREE_LEVEL);
                                        glUniform1ui(program->octree_resolution, gl.octree_resolution);
                                        glUniform1ui(program->DEBUG_level, batch->DEBUG_voxel_level);

                                        glBindImageTexture(0, gl.octree_nodes_texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);
                                        glBindImageTexture(1, gl.octree_diffuse_texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);

                                        glEnableVertexAttribArray(0);
                                        glEnableVertexAttribArray(1);
                                        glEnableVertexAttribArray(2);
                                        glEnableVertexAttribArray(3);
                                        glEnableVertexAttribArray(4);
                                        glEnableVertexAttribArray(5);

                                        glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(Vertex), (GLvoid *)(offset_of(Vertex, pos)));
                                        glVertexAttribPointer(1, 3, GL_FLOAT, false, sizeof(Vertex), (GLvoid *)(offset_of(Vertex, normal)));
                                        glVertexAttribPointer(2, 2, GL_FLOAT, false, sizeof(Vertex), (GLvoid *)(offset_of(Vertex, uv)));
                                        glVertexAttribPointer(3, 4, GL_FLOAT, true,  sizeof(Vertex), (GLvoid *)(offset_of(Vertex, color)));
                                        glVertexAttribIPointer(4, MAX_BONE_PER_VERTEX, GL_INT, sizeof(Vertex), (GLvoid *)(offset_of(Vertex, node_ids)));
                                        glVertexAttribPointer(5, MAX_BONE_PER_VERTEX, GL_FLOAT, false, sizeof(Vertex), (GLvoid *)(offset_of(Vertex, node_weights)));

                                        glBufferData(GL_ARRAY_BUFFER,
                                                     mesh->vertex_count * sizeof(Vertex),
                                                     mesh->vertices,
                                                     GL_DYNAMIC_DRAW);

                                        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                                                     mesh->index_count * sizeof(u32),
                                                     mesh->indices,
                                                     GL_DYNAMIC_DRAW);


                                        glDrawElements(GL_TRIANGLES, mesh->index_count, GL_UNSIGNED_INT, (void *)0);

                                        glDisableVertexAttribArray(0);
                                        glDisableVertexAttribArray(1);
                                        glDisableVertexAttribArray(2);
                                        glDisableVertexAttribArray(3);
                                        glDisableVertexAttribArray(4);
                                        glDisableVertexAttribArray(5);
                                    } break;

                                    INVALID_DEFAULT_CASE;
                            }
                        } break;

                    case eRender_Grass:
                        {
#if 1
                            Render_Grass *piece = (Render_Grass *)entity;

                            glBindBuffer(GL_ARRAY_BUFFER, gl.vbo);

                            glDisable(GL_CULL_FACE);

                            Grass_Program *program = &gl.grass_program;
                            s32 pid = program->id;
                            glUseProgram(pid);

                            glUniformMatrix4fv(program->mvp, 1, GL_TRUE, &group->camera->VP.e[0][0]);

                            glEnableVertexAttribArray(0);
                            glEnableVertexAttribArray(1);
                            glEnableVertexAttribArray(2);
                            glEnableVertexAttribArray(3);
                            glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(Vertex), (GLvoid *)(offset_of(Vertex, pos)));
                            glVertexAttribPointer(1, 3, GL_FLOAT, false, sizeof(Vertex), (GLvoid *)(offset_of(Vertex, normal)));
                            glVertexAttribPointer(2, 2, GL_FLOAT, false, sizeof(Vertex), (GLvoid *)(offset_of(Vertex, uv)));
                            glVertexAttribPointer(3, 4, GL_FLOAT, true,  sizeof(Vertex), (GLvoid *)(offset_of(Vertex, color)));

                            glUniform1f(program->time, piece->time);
                            glUniform1f(program->grass_max_vertex_y, piece->grass_max_vertex_y);
                            Mesh *mesh    = piece->mesh;
                            glBufferData(GL_ARRAY_BUFFER,
                                         mesh->vertex_count * sizeof(Vertex),
                                         mesh->vertices,
                                         GL_DYNAMIC_DRAW);
                            glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                                         mesh->index_count * sizeof(u32),
                                         mesh->indices,
                                         GL_DYNAMIC_DRAW);

                            glBindBuffer(GL_ARRAY_BUFFER, gl.grass_vbo);
                            glEnableVertexAttribArray(6);
                            glEnableVertexAttribArray(7);
                            glEnableVertexAttribArray(8);
                            glEnableVertexAttribArray(9);
                            glVertexAttribPointer(6, 4, GL_FLOAT, false, 4 * sizeof(v4), (GLvoid *)0);
                            glVertexAttribPointer(7, 4, GL_FLOAT, false, 4 * sizeof(v4), (GLvoid *)(sizeof(v4)));
                            glVertexAttribPointer(8, 4, GL_FLOAT, false, 4 * sizeof(v4), (GLvoid *)(2 * sizeof(v4)));
                            glVertexAttribPointer(9, 4, GL_FLOAT, false, 4 * sizeof(v4), (GLvoid *)(3 * sizeof(v4)));
                            glVertexAttribDivisor(6, 1);
                            glVertexAttribDivisor(7, 1);
                            glVertexAttribDivisor(8, 1);
                            glVertexAttribDivisor(9, 1);
                            glBufferData(GL_ARRAY_BUFFER, sizeof(m4x4) * piece->count, piece->world_transforms, GL_STATIC_DRAW);
                            glBindBuffer(GL_ARRAY_BUFFER, gl.vbo);

                            Bitmap *turbulence_map = piece->turbulence_map;
                            if (!turbulence_map->handle)
                            {
                                glGenTextures(1, &turbulence_map->handle);
                                glBindTexture(GL_TEXTURE_2D, turbulence_map->handle);
                                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, turbulence_map->width, turbulence_map->height,
                                             0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, (u8 *)turbulence_map->memory);

                                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                            }
                            glBindTexture(GL_TEXTURE_2D, turbulence_map->handle);
                            glDrawElementsInstanced(GL_TRIANGLES, mesh->index_count, GL_UNSIGNED_INT, (void *)0, piece->count);

                            glDisableVertexAttribArray(0);
                            glDisableVertexAttribArray(1);
                            glDisableVertexAttribArray(2);
                            glDisableVertexAttribArray(3);
                            glDisableVertexAttribArray(6);
                            glDisableVertexAttribArray(7);
                            glDisableVertexAttribArray(8);
                            glDisableVertexAttribArray(9);
#endif
                        } break;

                    case eRender_Star:
                        {
                            Render_Star *piece = (Render_Star *)entity;

                            Mesh *mesh = piece->mesh;

                            glBindBuffer(GL_ARRAY_BUFFER, gl.vbo);

                            Star_Program *program = &gl.star_program;
                            s32 pid = program->id;
                            glUseProgram(pid);

                            glUniformMatrix4fv(program->mvp, 1, GL_TRUE, &group->camera->VP.e[0][0]);
                            glUniform1f(program->time, piece->time);

                            glEnableVertexAttribArray(0);
                            glEnableVertexAttribArray(1);
                            glEnableVertexAttribArray(2);
                            glEnableVertexAttribArray(3);
                            glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(Vertex), (GLvoid *)(offset_of(Vertex, pos)));
                            glVertexAttribPointer(1, 3, GL_FLOAT, false, sizeof(Vertex), (GLvoid *)(offset_of(Vertex, normal)));
                            glVertexAttribPointer(2, 2, GL_FLOAT, false, sizeof(Vertex), (GLvoid *)(offset_of(Vertex, uv)));
                            glVertexAttribPointer(3, 4, GL_FLOAT, true,  sizeof(Vertex), (GLvoid *)(offset_of(Vertex, color)));

                            glBufferData(GL_ARRAY_BUFFER,
                                         mesh->vertex_count * sizeof(Vertex),
                                         mesh->vertices,
                                         GL_DYNAMIC_DRAW);
                            glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                                         mesh->index_count * sizeof(u32),
                                         mesh->indices,
                                         GL_DYNAMIC_DRAW);

                            glBindBuffer(GL_ARRAY_BUFFER, gl.star_vbo);
                            glEnableVertexAttribArray(6);
                            glEnableVertexAttribArray(7);
                            glEnableVertexAttribArray(8);
                            glEnableVertexAttribArray(9);
                            glVertexAttribPointer(6, 4, GL_FLOAT, false, 4 * sizeof(v4), (GLvoid *)0);
                            glVertexAttribPointer(7, 4, GL_FLOAT, false, 4 * sizeof(v4), (GLvoid *)(sizeof(v4)));
                            glVertexAttribPointer(8, 4, GL_FLOAT, false, 4 * sizeof(v4), (GLvoid *)(2 * sizeof(v4)));
                            glVertexAttribPointer(9, 4, GL_FLOAT, false, 4 * sizeof(v4), (GLvoid *)(3 * sizeof(v4)));
                            glVertexAttribDivisor(6, 1);
                            glVertexAttribDivisor(7, 1);
                            glVertexAttribDivisor(8, 1);
                            glVertexAttribDivisor(9, 1);
                            glBufferData(GL_ARRAY_BUFFER, sizeof(m4x4) * piece->count, piece->world_transforms, GL_STATIC_DRAW);
                            glBindBuffer(GL_ARRAY_BUFFER, gl.vbo);

                            glDrawElementsInstanced(GL_TRIANGLES, mesh->index_count, GL_UNSIGNED_INT, (void *)0, piece->count);

                            glDisableVertexAttribArray(0);
                            glDisableVertexAttribArray(1);
                            glDisableVertexAttribArray(2);
                            glDisableVertexAttribArray(3);
                            glDisableVertexAttribArray(6);
                            glDisableVertexAttribArray(7);
                            glDisableVertexAttribArray(8);
                            glDisableVertexAttribArray(9);

                        } break;

                    case eRender_Bitmap:
                        {
                            Render_Bitmap *piece  = (Render_Bitmap *)entity;

                            glBindBuffer(GL_ARRAY_BUFFER, gl.vbo);

                            Sprite_Program *program = &gl.sprite_program;
                            s32 pid = program->id;
                            glUseProgram(pid);

                            glUniformMatrix4fv(program->mvp, 1, GL_TRUE, &group->camera->VP.e[0][0]);
                            glUniform4fv(program->color, 1, &piece->color.r);

                            glEnableVertexAttribArray(0);
                            glEnableVertexAttribArray(2);

                            glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(Textured_Vertex), (GLvoid *)offset_of(Textured_Vertex, pos));
                            glVertexAttribPointer(2, 2, GL_FLOAT, false, sizeof(Textured_Vertex), (GLvoid *)offset_of(Textured_Vertex, uv));

                            gl_bind_texture(piece->bitmap);

                            v3 min = piece->min;
                            v3 max = piece->max;

                            Textured_Vertex v[4];
                            v[0].pos = V3(max.x, min.y, 0);
                            v[0].uv  = V2(1, 0);

                            v[1].pos = max;
                            v[1].uv  = V2(1, 1);

                            v[2].pos = min;
                            v[2].uv  = V2(0, 0);

                            v[3].pos = V3(min.x, max.y, 0);
                            v[3].uv  = V2(0, 1);

                            glBufferData(GL_ARRAY_BUFFER,
                                         array_count(v) * sizeof(*v), v,
                                         GL_STATIC_DRAW);
                            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

                            glDisableVertexAttribArray(0);
                            glDisableVertexAttribArray(2);
                        } break;
                }
            }
        }
#else
        GL(glBindBuffer(GL_ARRAY_BUFFER, gl.vbo));

        Defer_Program *dp = &gl.defer_program;
        s32 dpid = dp->id;
        GL(glUseProgram(dpid));

        // G-Buffer
        GL(glActiveTexture(GL_TEXTURE1));
        GL(glBindTexture(GL_TEXTURE_2D, gb->Pid));
        GL(glActiveTexture(GL_TEXTURE2));
        GL(glBindTexture(GL_TEXTURE_2D, gb->Nid));
        GL(glActiveTexture(GL_TEXTURE3));
        GL(glBindTexture(GL_TEXTURE_2D, gb->Cid));
        GL(glActiveTexture(GL_TEXTURE0));

        GL(glBindImageTexture(0, gl.octree_nodes_texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI));
        GL(glBindImageTexture(1, gl.octree_diffuse_texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI));

        GL(glUniformMatrix4fv(dp->voxel_P, 1, GL_TRUE, &voxelize_clip_P.e[0][0]));
        GL(glUniform3fv(dp->cam_P, 1, (GLfloat *)&cam_P));
        GL(glUniform3fv(dp->DEBUG_light_P, 1, (GLfloat *)&light_P));
        GL(glUniform3fv(dp->DEBUG_light_color, 1, (GLfloat *)&light_color));
        GL(glUniform1f(dp->DEBUG_light_strength, light_strength));
        GL(glUniform1f(dp->voxel_in_meter, voxel_in_meter));
        GL(glUniform1ui(dp->octree_level, OCTREE_LEVEL));
        GL(glUniform1ui(dp->octree_resolution, gl.octree_resolution));
        GL(glUniform1ui(dp->svogi_on, batch->svogi_on));

        // Draw full quad on the screen.
        f32 vertices[] = { // P, UV
            1.0f, -1.0f, 0.0f,      1.0f,  0.0f,
            1.0f,  1.0f, 0.0f,      1.0f,  1.0f,
           -1.0f, -1.0f, 0.0f,      0.0f,  0.0f,
           -1.0f,  1.0f, 0.0f,      0.0f,  1.0f,
        };

        GL(glEnableVertexAttribArray(0));
        GL(glEnableVertexAttribArray(2));

        GL(glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(f32) * 5, (GLvoid *)(0)));
        GL(glVertexAttribPointer(2, 2, GL_FLOAT, false, sizeof(f32) * 5, (GLvoid *)(sizeof(f32) * 3)));

        GL(glBufferData(GL_ARRAY_BUFFER,
                        sizeof(f32) * array_count(vertices),
                        vertices,
                        GL_DYNAMIC_DRAW));
        GL(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));
        GL(glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT));

        GL(glDisableVertexAttribArray(0));
        GL(glDisableVertexAttribArray(2));

        GL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
        GL(glEnable(GL_BLEND));

        GL(glClearDepth(1));
        GL(glClear(GL_DEPTH_BUFFER_BIT));

        // Draw else... overlay..
        for (Render_Group *group = (Render_Group *)batch->base;
             (u8 *)group < (u8 *)batch->base + batch->used;
             ++group)
        {
            for (u8 *at = group->base;
                 at < group->base + group->used;
                 at += ((Render_Entity_Header *)at)->size)
            {
                Render_Entity_Header *entity = (Render_Entity_Header *)at;
                switch (entity->type)
                {
                    case eRender_Bitmap:
                        {
                            Render_Bitmap *piece  = (Render_Bitmap *)entity;

                            glBindBuffer(GL_ARRAY_BUFFER, gl.vbo);

                            Sprite_Program *program = &gl.sprite_program;
                            s32 pid = program->id;
                            glUseProgram(pid);

                            glUniformMatrix4fv(program->mvp, 1, GL_TRUE, &group->VP.e[0][0]);
                            glUniform4fv(program->color, 1, &piece->color.r);

                            glEnableVertexAttribArray(0);
                            glEnableVertexAttribArray(2);

                            glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(Textured_Vertex), (GLvoid *)offset_of(Textured_Vertex, pos));
                            glVertexAttribPointer(2, 2, GL_FLOAT, false, sizeof(Textured_Vertex), (GLvoid *)offset_of(Textured_Vertex, uv));

                            gl_bind_texture(piece->bitmap, Gl_Bind_Texture_White);

                            v3 min = piece->min;
                            v3 max = piece->max;

                            Textured_Vertex v[4];
                            v[0].pos = V3(max.x, min.y, 0);
                            v[0].uv  = V2(1, 0);

                            v[1].pos = max;
                            v[1].uv  = V2(1, 1);

                            v[2].pos = min;
                            v[2].uv  = V2(0, 0);

                            v[3].pos = V3(min.x, max.y, 0);
                            v[3].uv  = V2(0, 1);

                            glBufferData(GL_ARRAY_BUFFER,
                                         array_count(v) * sizeof(*v), v,
                                         GL_STATIC_DRAW);
                            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

                            glDisableVertexAttribArray(0);
                            glDisableVertexAttribArray(2);
                        } break;
                }
            }
        }
#endif
#endif
