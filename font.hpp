#ifndef FONT_HPP
#define FONT_HPP

#define u32 unsigned int
#define elif else if

namespace font {
    const int FONT_RANGE = 400;

    const char* vertShader =
                R"(
                #version 330 core

                uniform mat4 uProjection;
                uniform mat4 uTransform;
                uniform vec4 uColor;

                layout (location = 0) in vec4 pos;
                layout (location = 1) in vec2 vTexCoord;

                out vec4 fColor;
                out vec2 fUV;

                void main()
                {
                    gl_Position = uProjection * uTransform * pos;
                    fColor = uColor;
                    fUV = vTexCoord;
                }
                )";

    const char* fragShader =
                R"(
                #version 330 core

                uniform sampler2D mainTex;

                in vec4 fColor;
                in vec2 fUV;

                out vec4 fragColor;

                void main()
                {
                    float isOpaque = texture(mainTex, fUV).a;
                    fragColor = vec4(fColor.r, fColor.g, fColor.b, fColor.a * isOpaque);
                }
                )";
     
    /* @TODO do something else with it, store the font shader in gl.hpp? */
    struct { GLuint vert_id, frag_id, program_id; } Shader = {0, 0, 0};

    void CreateProgram () {
        Shader.vert_id = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(Shader.vert_id, 1, &vertShader, NULL);
        glCompileShader(Shader.vert_id);

        Shader.frag_id = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(Shader.frag_id, 1, &fragShader, NULL);
        glCompileShader(Shader.frag_id);

        Shader.program_id = glCreateProgram();
        glAttachShader(Shader.program_id, Shader.vert_id);
        glAttachShader(Shader.program_id, Shader.frag_id);
        glLinkProgram(Shader.program_id); 
    }

    struct Font {
        int size;
        int atlasSize;
        GLuint tex;
        stbtt_packedchar charData[FONT_RANGE];
    };

    Font* Font_I (int fontSize) {
        Font* font = new Font();
        font->size = fontSize;
        font->atlasSize = 1024;

        unsigned char* fontData  = new unsigned char[1<<20]; /* 1<<20 = 1024*1024 ~ 1MB max file size of font*/
        unsigned char* atlasData = new unsigned char[font->atlasSize * font->atlasSize];
        FILE* fontFile = fopen("font/cmunrm.ttf", "rb"); /* @TODO make the font name an argument */
        fread(fontData, sizeof(char), 1<<20, fontFile);
        fclose(fontFile);

        stbtt_pack_context context;
        if (!stbtt_PackBegin(&context, atlasData, font->atlasSize, font->atlasSize, 0, 1, nullptr)) {
            std::cout<<"ERROR: Failed to initialize font";
        }

        stbtt_PackSetOversampling(&context, 1, 1);
        if (!stbtt_PackFontRange(&context, fontData, 0, font->size, 32, FONT_RANGE, font->charData)) { /* 32 is the code for space character; the pack font starts from it */
            std::cout<<"ERROR: Failed to pack font";
        }
        stbtt_PackEnd(&context);

        glGenTextures(1, &font->tex);
        glBindTexture(GL_TEXTURE_2D, font->tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glActiveTexture(GL_TEXTURE0);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, font->atlasSize, font->atlasSize, 0, GL_ALPHA, GL_UNSIGNED_BYTE, atlasData);

        /* resetting section */
        glBindTexture(GL_TEXTURE_2D, 0);

        delete[] fontData;
        delete[] atlasData;
        return font;
    }

    void Font_D (Font*& font) {
        if (font == nullptr) { return; }

        glDeleteTextures(1, &font->tex);
        delete font; 
        font = nullptr;
    }

}

#undef u32
#undef elif

#endif // font.hpp
