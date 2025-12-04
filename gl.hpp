#ifndef GL_HPP
#define GL_HPP

namespace gl {
    /* @TODO store locations of uniforms, store programs and their binds */
    struct Program {
        GLfloat program_id, vao_id, vbo_id, ebo_id, tex_id;
    };
    void UpdateProjectionUniforms () {
        
    }
}

#endif // gl.hpp
