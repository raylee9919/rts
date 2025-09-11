
void parse_member_Camera(Camera *e, Parser *parser) {
    Token *token = peek_token(parser);
    Assert(token->type == TOKEN_IDENT);

    if (string_equal(token->scratch_buffer, token->scratch_buffer_length, "position")) {
        eat_token(parser);

        e->position.x = parse_float(parser);
        e->position.y = parse_float(parser);
        e->position.z = parse_float(parser);

        return;
    } 

    if (string_equal(token->scratch_buffer, token->scratch_buffer_length, "orientation")) {
        eat_token(parser);

        e->orientation.w = parse_float(parser);
        e->orientation.x = parse_float(parser);
        e->orientation.y = parse_float(parser);
        e->orientation.z = parse_float(parser);

        return;
    } 

    if (string_equal(token->scratch_buffer, token->scratch_buffer_length, "scaling")) {
        eat_token(parser);

        e->scaling.x = parse_float(parser);
        e->scaling.y = parse_float(parser);
        e->scaling.z = parse_float(parser);

        return;
    } 

    if (string_equal(token->scratch_buffer, token->scratch_buffer_length, "radius")) {
        eat_token(parser);

        e->radius = parse_float(parser);

        return;
    } 

    if (string_equal(token->scratch_buffer, token->scratch_buffer_length, "type")) {
        eat_token(parser);

        e->type = parse_integer(parser);

        return;
    } 

    if (string_equal(token->scratch_buffer, token->scratch_buffer_length, "focal_length")) {
        eat_token(parser);

        e->focal_length = parse_float(parser);

        return;
    } 

    if (string_equal(token->scratch_buffer, token->scratch_buffer_length, "width")) {
        eat_token(parser);

        e->width = parse_float(parser);

        return;
    } 

    if (string_equal(token->scratch_buffer, token->scratch_buffer_length, "height")) {
        eat_token(parser);

        e->height = parse_float(parser);

        return;
    } 

    if (string_equal(token->scratch_buffer, token->scratch_buffer_length, "N")) {
        eat_token(parser);

        e->N = parse_float(parser);

        return;
    } 

    if (string_equal(token->scratch_buffer, token->scratch_buffer_length, "F")) {
        eat_token(parser);

        e->F = parse_float(parser);

        return;
    } 

    else {
        INVALID_CODE_PATH;
    }
}

internal void
parse_Camera(Parser *parser) 
{
    Camera *e = push_entity(parser->game_state->world, &parser->game_state->world_arena, Camera, {});

    eat_token(parser);

    s32 version = parse_integer(parser);
    if (version == 1) {
        s32 id = parse_integer(parser);
        while (1) {
            Token *token = peek_token(parser);
            if (token->type == TOKEN_SEMICOLON) {
                eat_token(parser);
                parse_member_Camera(e, parser);
            } else {
                break;
            }
        }
    } else {
        INVALID_CODE_PATH;
    }

    e->init(parser->game_state);
}

void parse_member_Crate(Crate *e, Parser *parser) {
    Token *token = peek_token(parser);
    Assert(token->type == TOKEN_IDENT);

    if (string_equal(token->scratch_buffer, token->scratch_buffer_length, "position")) {
        eat_token(parser);

        e->position.x = parse_float(parser);
        e->position.y = parse_float(parser);
        e->position.z = parse_float(parser);

        return;
    } 

    if (string_equal(token->scratch_buffer, token->scratch_buffer_length, "orientation")) {
        eat_token(parser);

        e->orientation.w = parse_float(parser);
        e->orientation.x = parse_float(parser);
        e->orientation.y = parse_float(parser);
        e->orientation.z = parse_float(parser);

        return;
    } 

    if (string_equal(token->scratch_buffer, token->scratch_buffer_length, "scaling")) {
        eat_token(parser);

        e->scaling.x = parse_float(parser);
        e->scaling.y = parse_float(parser);
        e->scaling.z = parse_float(parser);

        return;
    } 

    if (string_equal(token->scratch_buffer, token->scratch_buffer_length, "radius")) {
        eat_token(parser);

        e->radius = parse_float(parser);

        return;
    } 
}

internal void
parse_Crate(Parser *parser) 
{
    Crate *e = push_entity(parser->game_state->world, &parser->game_state->world_arena, Crate, {});

    eat_token(parser);

    s32 version = parse_integer(parser);
    if (version == 1) {
        s32 id = parse_integer(parser);
        while (1) {
            Token *token = peek_token(parser);
            if (token->type == TOKEN_SEMICOLON) {
                eat_token(parser);
                parse_member_Crate(e, parser);
            } else {
                break;
            }
        }
    } else {
        INVALID_CODE_PATH;
    }

    e->init(parser->game_state);
}

void parse_member_Ground(Ground *e, Parser *parser) {
    Token *token = peek_token(parser);
    Assert(token->type == TOKEN_IDENT);

    if (string_equal(token->scratch_buffer, token->scratch_buffer_length, "position")) {
        eat_token(parser);

        e->position.x = parse_float(parser);
        e->position.y = parse_float(parser);
        e->position.z = parse_float(parser);

        return;
    } 

    if (string_equal(token->scratch_buffer, token->scratch_buffer_length, "orientation")) {
        eat_token(parser);

        e->orientation.w = parse_float(parser);
        e->orientation.x = parse_float(parser);
        e->orientation.y = parse_float(parser);
        e->orientation.z = parse_float(parser);

        return;
    } 

    if (string_equal(token->scratch_buffer, token->scratch_buffer_length, "scaling")) {
        eat_token(parser);

        e->scaling.x = parse_float(parser);
        e->scaling.y = parse_float(parser);
        e->scaling.z = parse_float(parser);

        return;
    } 

    if (string_equal(token->scratch_buffer, token->scratch_buffer_length, "radius")) {
        eat_token(parser);

        e->radius = parse_float(parser);

        return;
    } 

    if (string_equal(token->scratch_buffer, token->scratch_buffer_length, "uv_scale")) {
        eat_token(parser);

        e->uv_scale = parse_float(parser);

        return;
    } 

    else {
        INVALID_CODE_PATH;
    }
}

internal void
parse_Ground(Parser *parser) 
{
    Ground *e = push_entity(parser->game_state->world, &parser->game_state->world_arena, Ground, {});

    eat_token(parser);

    s32 version = parse_integer(parser);
    if (version == 1) {
        s32 id = parse_integer(parser);
        while (1) {
            Token *token = peek_token(parser);
            if (token->type == TOKEN_SEMICOLON) {
                eat_token(parser);
                parse_member_Ground(e, parser);
            } else {
                break;
            }
        }
    } else {
        INVALID_CODE_PATH;
    }

    e->init(parser->game_state);
}

void parse_member_Rock(Rock *e, Parser *parser) {
    Token *token = peek_token(parser);
    Assert(token->type == TOKEN_IDENT);

    if (string_equal(token->scratch_buffer, token->scratch_buffer_length, "position")) {
        eat_token(parser);

        e->position.x = parse_float(parser);
        e->position.y = parse_float(parser);
        e->position.z = parse_float(parser);

        return;
    } 

    if (string_equal(token->scratch_buffer, token->scratch_buffer_length, "orientation")) {
        eat_token(parser);

        e->orientation.w = parse_float(parser);
        e->orientation.x = parse_float(parser);
        e->orientation.y = parse_float(parser);
        e->orientation.z = parse_float(parser);

        return;
    } 

    if (string_equal(token->scratch_buffer, token->scratch_buffer_length, "scaling")) {
        eat_token(parser);

        e->scaling.x = parse_float(parser);
        e->scaling.y = parse_float(parser);
        e->scaling.z = parse_float(parser);

        return;
    } 

    if (string_equal(token->scratch_buffer, token->scratch_buffer_length, "radius")) {
        eat_token(parser);

        e->radius = parse_float(parser);

        return;
    } 
}

internal void
parse_Rock(Parser *parser) 
{
    Rock *e = push_entity(parser->game_state->world, &parser->game_state->world_arena, Rock, {});

    eat_token(parser);

    s32 version = parse_integer(parser);
    if (version == 1) {
        s32 id = parse_integer(parser);
        while (1) {
            Token *token = peek_token(parser);
            if (token->type == TOKEN_SEMICOLON) {
                eat_token(parser);
                parse_member_Rock(e, parser);
            } else {
                break;
            }
        }
    } else {
        INVALID_CODE_PATH;
    }

    e->init(parser->game_state);
}

void parse_member_Xbot(Xbot *e, Parser *parser) {
    Token *token = peek_token(parser);
    Assert(token->type == TOKEN_IDENT);

    if (string_equal(token->scratch_buffer, token->scratch_buffer_length, "position")) {
        eat_token(parser);

        e->position.x = parse_float(parser);
        e->position.y = parse_float(parser);
        e->position.z = parse_float(parser);

        return;
    } 

    if (string_equal(token->scratch_buffer, token->scratch_buffer_length, "orientation")) {
        eat_token(parser);

        e->orientation.w = parse_float(parser);
        e->orientation.x = parse_float(parser);
        e->orientation.y = parse_float(parser);
        e->orientation.z = parse_float(parser);

        return;
    } 

    if (string_equal(token->scratch_buffer, token->scratch_buffer_length, "scaling")) {
        eat_token(parser);

        e->scaling.x = parse_float(parser);
        e->scaling.y = parse_float(parser);
        e->scaling.z = parse_float(parser);

        return;
    } 

    if (string_equal(token->scratch_buffer, token->scratch_buffer_length, "radius")) {
        eat_token(parser);

        e->radius = parse_float(parser);

        return;
    } 

    if (string_equal(token->scratch_buffer, token->scratch_buffer_length, "speed")) {
        eat_token(parser);

        e->speed = parse_float(parser);

        return;
    } 

    if (string_equal(token->scratch_buffer, token->scratch_buffer_length, "hp")) {
        eat_token(parser);

        e->hp = parse_float(parser);

        return;
    } 

    if (string_equal(token->scratch_buffer, token->scratch_buffer_length, "controlled")) {
        eat_token(parser);

        e->controlled = parse_integer(parser);

        return;
    } 

    else {
        INVALID_CODE_PATH;
    }
}

internal void
parse_Xbot(Parser *parser) 
{
    Xbot *e = push_entity(parser->game_state->world, &parser->game_state->world_arena, Xbot, {});

    eat_token(parser);

    s32 version = parse_integer(parser);
    if (version == 1) {
        s32 id = parse_integer(parser);
        while (1) {
            Token *token = peek_token(parser);
            if (token->type == TOKEN_SEMICOLON) {
                eat_token(parser);
                parse_member_Xbot(e, parser);
            } else {
                break;
            }
        }
    } else {
        INVALID_CODE_PATH;
    }

    e->init(parser->game_state);
}
