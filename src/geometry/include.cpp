// Copyright Seong Woo Lee. All Rights Reserved.

void geo_make_cube(void *out_vertices, int vertex_stride,
                   int position_offset,
                   int normal_offset,
                   int uv_offset,
                   void *out_indices, int index_size)
{
    uint8_t *vertices = (uint8_t *)out_vertices;

    // +X
    {
        float p[4][3] = {
            { 1, -1, -1 },
            { 1,  1, -1 },
            { 1,  1,  1 },
            { 1, -1,  1 },
        };
        float n[3] = { 1, 0, 0 };

        for (int i = 0; i < 4; ++i) {
            uint8_t *v = vertices + (0 + i) * vertex_stride;

            float uv[2] = {
                (i == 2 || i == 3) ? 1.0f : 0.0f,
                (i == 0 || i == 3) ? 1.0f : 0.0f,
            };

            memcpy(v + position_offset, p[i], sizeof(p[i]));
            memcpy(v + normal_offset, n, sizeof(n));
            memcpy(v + uv_offset, uv, sizeof(uv));
        }
    }

    // -X
    {
        float p[4][3] = {
            { -1, -1,  1 },
            { -1,  1,  1 },
            { -1,  1, -1 },
            { -1, -1, -1 },
        };
        float n[3] = { -1, 0, 0 };

        for (int i = 0; i < 4; ++i) {
            uint8_t *v = vertices + (4 + i) * vertex_stride;

            float uv[2] = {
                (i == 2 || i == 3) ? 1.0f : 0.0f,
                (i == 0 || i == 3) ? 1.0f : 0.0f,
            };

            memcpy(v + position_offset, p[i], sizeof(p[i]));
            memcpy(v + normal_offset, n, sizeof(n));
            memcpy(v + uv_offset, uv, sizeof(uv));
        }
    }

    // +Y
    {
        float p[4][3] = {
            { -1, 1, -1 },
            { -1, 1,  1 },
            {  1, 1,  1 },
            {  1, 1, -1 },
        };
        float n[3] = { 0, 1, 0 };

        for (int i = 0; i < 4; ++i) {
            uint8_t *v = vertices + (8 + i) * vertex_stride;

            float uv[2] = {
                (i == 2 || i == 3) ? 1.0f : 0.0f,
                (i == 0 || i == 3) ? 1.0f : 0.0f,
            };

            memcpy(v + position_offset, p[i], sizeof(p[i]));
            memcpy(v + normal_offset, n, sizeof(n));
            memcpy(v + uv_offset, uv, sizeof(uv));
        }
    }

    // -Y
    {
        float p[4][3] = {
            { -1, -1,  1 },
            { -1, -1, -1 },
            {  1, -1, -1 },
            {  1, -1,  1 },
        };
        float n[3] = { 0, -1, 0 };

        for (int i = 0; i < 4; ++i) {
            uint8_t *v = vertices + (12 + i) * vertex_stride;

            float uv[2] = {
                (i == 2 || i == 3) ? 1.0f : 0.0f,
                (i == 0 || i == 3) ? 1.0f : 0.0f,
            };

            memcpy(v + position_offset, p[i], sizeof(p[i]));
            memcpy(v + normal_offset, n, sizeof(n));
            memcpy(v + uv_offset, uv, sizeof(uv));
        }
    }

    // +Z
    {
        float p[4][3] = {
            {  1, -1, 1 },
            {  1,  1, 1 },
            { -1,  1, 1 },
            { -1, -1, 1 },
        };
        float n[3] = { 0, 0, 1 };

        for (int i = 0; i < 4; ++i) {
            uint8_t *v = vertices + (16 + i) * vertex_stride;

            float uv[2] = {
                (i == 2 || i == 3) ? 1.0f : 0.0f,
                (i == 0 || i == 3) ? 1.0f : 0.0f,
            };

            memcpy(v + position_offset, p[i], sizeof(p[i]));
            memcpy(v + normal_offset, n, sizeof(n));
            memcpy(v + uv_offset, uv, sizeof(uv));
        }
    }

    // -Z
    {
        float p[4][3] = {
            { -1, -1, -1 },
            { -1,  1, -1 },
            {  1,  1, -1 },
            {  1, -1, -1 },
        };
        float n[3] = { 0, 0, -1 };

        for (int i = 0; i < 4; ++i) {
            uint8_t *v = vertices + (20 + i) * vertex_stride;

            float uv[2] = {
                (i == 2 || i == 3) ? 1.0f : 0.0f,
                (i == 0 || i == 3) ? 1.0f : 0.0f,
            };

            memcpy(v + position_offset, p[i], sizeof(p[i]));
            memcpy(v + normal_offset, n, sizeof(n));
            memcpy(v + uv_offset, uv, sizeof(uv));
        }
    }

    uint32_t indices[] = {
        0,  1,  2,   0,  2,  3,
        4,  5,  6,   4,  6,  7,
        8,  9, 10,   8, 10, 11,
        12, 13, 14,  12, 14, 15,
        16, 17, 18,  16, 18, 19,
        20, 21, 22,  20, 22, 23,
    };

    if (index_size == 2) {
        uint16_t *out = (uint16_t *)out_indices;

        for (int i = 0; i < 36; ++i)
            out[i] = (uint16_t)indices[i];
    } else {
        uint32_t *out = (uint32_t *)out_indices;

        for (int i = 0; i < 36; ++i)
            out[i] = indices[i];
    }
}
