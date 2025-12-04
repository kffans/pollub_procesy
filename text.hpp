#ifndef TEXT_HPP
#define TEXT_HPP

#define u32 unsigned int
#define elif else if

/* @NOTE depends on lin.hpp, font.hpp and the PROJECTION variable */
namespace text {
    struct Color { float r, g, b, a; };

    struct Text {
        Mat4 transform;
        Color color;
        Vec3 lastCharPos;
        u32 length;
        GLuint vao, vbo, ebo, tex, program;
    };

    int utf8IndexFromCodePoint (char32_t codepoint);
    std::basic_string<char32_t> convertStr8ToStr32 (std::string str8);

    Text* Text_I (font::Font* font, std::string str8) {
        Text* text = new Text();
        text->transform = MAT4_IDENTITY;
        text->color = { 0.0f, 0.0f, 0.0f, 1.0f };
        text->lastCharPos = { 0.0f, 0.0f, 0.0f };
        text->length = 0;
        text->vao = 0; text->vbo = 0; text->ebo = 0, text->tex = 0; text->program = font::Shader.program_id;
        if (font == nullptr) { return text; }
        u32 lineBreak_c = 0;

        std::basic_string<char32_t> str32 = convertStr8ToStr32(str8);
        text->length = str32.length();
        /* @TODO check here which unicode characters from the text aren't in the font atlas */
        
        float x = 0, y = 0, xStart = 0, yStart = 0;

        float points[4 * 5 * text->length]; int p_i = 0;
        u32 indices[2 * 3 * text->length]; int i_i = 0;
        const int textOffsetY = font->size/2;
        
        for (u32 i = 0; i < text->length; i++) {
            int char_i = utf8IndexFromCodePoint(str32[i]) - 32; /* 32 because it is ascii index of first character (which is space) */
            if (char_i < 0 || char_i >= font::FONT_RANGE) { char_i = 0; }

            stbtt_aligned_quad q;
            stbtt_GetPackedQuad(font->charData, font->atlasSize, font->atlasSize, char_i, &x, &y, &q, 1);
            /* q.x0, q.x1, q.y0 and q.y1 are in pixels */

            points[p_i++] = q.x0; points[p_i++] = -(q.y0 + textOffsetY); points[p_i++] = 0.0f; points[p_i++] = q.s0; points[p_i++] = q.t0; /* top left */
            points[p_i++] = q.x1; points[p_i++] = -(q.y0 + textOffsetY); points[p_i++] = 0.0f; points[p_i++] = q.s1; points[p_i++] = q.t0; /* top right */
            points[p_i++] = q.x1; points[p_i++] = -(q.y1 + textOffsetY); points[p_i++] = 0.0f; points[p_i++] = q.s1; points[p_i++] = q.t1; /* bot right */
            points[p_i++] = q.x0; points[p_i++] = -(q.y1 + textOffsetY); points[p_i++] = 0.0f; points[p_i++] = q.s0; points[p_i++] = q.t1; /* bot left */

            indices[i_i++] = 0 + i*4; indices[i_i++] = 1 + i*4; indices[i_i++] = 2 + i*4; /* top right triangle */
            indices[i_i++] = 2 + i*4; indices[i_i++] = 3 + i*4; indices[i_i++] = 0 + i*4; /* bot left triangle */

            if (str32[i] == U'\n') {
                lineBreak_c++;
                x = xStart;
                y = yStart + font->size * lineBreak_c; /* @TODO can set line height here */
            }
        }

        GLuint vao = 0;
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        GLuint vbo = 0;
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, 4 * 5 * text->length * sizeof(float), points, GL_STATIC_DRAW);

        GLuint ebo = 0;
        glGenBuffers(1, &ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, 2 * 3 * text->length * sizeof(u32), indices, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0); /* position data */
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float))); /* texture data */

        /* resetting section */
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);

        text->lastCharPos = {x, -y, 0.0f};
        text->vao = vao; text->vbo = vbo; text->ebo = ebo; text->tex = font->tex;
        return text;
    }

    void Text_D (Text*& text) {
        if (text == nullptr) { return; }

        if (text->vao != 0) glDeleteVertexArrays(1, &text->vao);
        if (text->vbo != 0) glDeleteBuffers(1, &text->vbo);
        if (text->ebo != 0) glDeleteBuffers(1, &text->ebo);
        delete text;
        text = nullptr;
    }
    
    void Draw (Text* text) {
        glUseProgram(text->program);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glBindTexture(GL_TEXTURE_2D, text->tex);
        
        GLint uniformLocationProjection = glGetUniformLocation(text->program, "uProjection");
        glUniformMatrix4fv(uniformLocationProjection, 1, GL_TRUE, PROJECTION.begin());

        GLint uniformLocationTransform = glGetUniformLocation(text->program, "uTransform");
        glUniformMatrix4fv(uniformLocationTransform, 1, GL_TRUE, text->transform.begin());

        GLint uniformLocationColor = glGetUniformLocation(text->program, "uColor");
        glUniform4f(uniformLocationColor, text->color.r, text->color.g, text->color.b, text->color.a);

        glBindVertexArray(text->vao);
        glBindBuffer(GL_ARRAY_BUFFER, text->vbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, text->ebo);
       
        glDrawElements(GL_TRIANGLES, 6 * text->length, GL_UNSIGNED_INT, nullptr);

        /* resetting section */
        glBlendFunc(GL_ONE, GL_ZERO);
        glDisable(GL_BLEND);
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glUseProgram(0);
    }

    /* UTF-8 */
    int utf8IndexFromCodePoint (char32_t codepoint) {
        int index = 0;
        int code = (int)codepoint;
        if ((code & (0b11110000<<24)) == (0b11110000<<24)) {
            index =  code & (0b00111111);
            index += (code & (0b00111111<<8))>>2;
            index += (code & (0b00111111<<16))>>4;
            index += (code & (0b00000111<<24))>>6;
        }
        elif ((code & (0b11100000<<16)) == (0b11100000<<16)) {
            index =  code & (0b00111111);
            index += (code & (0b00111111<<8))>>2;
            index += (code & (0b00001111<<16))>>4;
        }
        elif ((code & (0b11000000<<8)) == (0b11000000<<8)) {
            index =  code & (0b00111111);
            index += (code & (0b00011111<<8))>>2;
        }
        else {
            index = code;
        }
        return index;
    }
    std::basic_string<char32_t> convertStr8ToStr32 (std::string str) {
        std::basic_string<char32_t> str32 = U"";
        const unsigned char* str8 = (const unsigned char*)str.c_str();
        u32 str8_s = str.length();
        u32 char32_b = 0;
        for (u32 i = 0; i < str8_s; i++) {
            if ((str8[i] & 0b11110000) == 0b11110000 && i+3 < str8_s) {
                char32_b =  str8[i]<<24; i++;
                char32_b += str8[i]<<16; i++;
                char32_b += str8[i]<<8;  i++;
                char32_b += str8[i];
            }
            elif ((str8[i] & 0b11100000) == 0b11100000 && i+2 < str8_s) {
                char32_b =  str8[i]<<16; i++;
                char32_b += str8[i]<<8;  i++;
                char32_b += str8[i];
            }
            elif ((str8[i] & 0b11000000) == 0b11000000 && i+1 < str8_s) {
                char32_b =  str8[i]<<8;  i++;
                char32_b += str8[i];
            }
            else {
                char32_b = str8[i];
            }
            str32 += (char32_t)char32_b;
        }
        return str32;
    }
}

#undef u32
#undef elif

#endif // text.hpp
