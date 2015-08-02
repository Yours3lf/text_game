#pragma once

#include "util.h"

class post_process
{
  GLuint main_fbo;
  GLuint color_tex;
  GLuint pingpong_tex;
  GLuint attribute_tex;
  GLuint motion_tex;
  GLuint downsample_shader;
  GLuint gauss_shader;
  GLuint radial_shader;
  GLuint display_shader;
  GLuint ss_quad;
  GLuint lookup_tex;
  vec2 screensize;

  void set_workgroup_size( vec2& gws, vec2& lws, vec2& dispatch_size, const vec2& size )
  {
    //set up work group sizes
    unsigned local_ws[2] = { 16, 16 };
    unsigned global_ws[2];
    unsigned gw = 0, gh = 0, count = 1;

    while( gw < size.x )
    {
      gw = local_ws[0] * count;
      count++;
    }

    count = 1;

    while( gh < size.y )
    {
      gh = local_ws[1] * count;
      count++;
    }

    global_ws[0] = gw;
    global_ws[1] = gh;

    gws = vec2( global_ws[0], global_ws[1] );
    lws = vec2( local_ws[0], local_ws[1] );
    dispatch_size = gws / lws;
  }

  void gen_mipmaps( GLuint texture, GLenum internal_format, const vec2& size, unsigned miplevels )
  {
    glUseProgram( downsample_shader );

    vec2 s = size;
    s *= 0.5f;

    for( int d = 1; d < miplevels; ++d )
    {
      glBindImageTexture( 0, texture, d - 1, GL_FALSE, 0, GL_READ_ONLY, internal_format );
      glBindImageTexture( 1, texture, d, GL_FALSE, 0, GL_WRITE_ONLY, internal_format );

      vec2 dispatch_size, gws, lws;
      set_workgroup_size( gws, lws, dispatch_size, s );

      glDispatchCompute( dispatch_size.x, dispatch_size.y, 1 );

      glMemoryBarrier( GL_SHADER_IMAGE_ACCESS_BARRIER_BIT );

      s *= 0.5f;
    }
  }

  void gauss_blur( const vec2& dir, const vec2& size, GLuint src_tex, int src_level, GLuint dst_tex, int dst_level )
  {
    glUseProgram( gauss_shader );

    glUniform2fv( 0, 1, &dir.x );

    glBindImageTexture( 0, src_tex, src_level, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8 );
    glBindImageTexture( 1, dst_tex, dst_level, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8 );

    vec2 dispatch_size, gws, lws;
    set_workgroup_size( gws, lws, dispatch_size, size );

    glDispatchCompute( dispatch_size.x, dispatch_size.y, 1 );

    glMemoryBarrier( GL_SHADER_IMAGE_ACCESS_BARRIER_BIT );
  }

  void radial_blur( const vec2& center, float density, float positive_weight, float negative_weight, float decay, const vec2& size, GLuint src_tex, int src_level, GLuint dst_tex, int dst_level )
  {
    glUseProgram( radial_shader );

    glUniform2fv( 0, 1, &center.x );
    glUniform1f( 1, density );
    glUniform1f( 2, positive_weight );
    glUniform1f( 3, negative_weight );
    glUniform1f( 4, decay );

    glBindImageTexture( 0, src_tex, src_level, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8 );
    glBindImageTexture( 1, dst_tex, dst_level, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8 );

    glActiveTexture( GL_TEXTURE2 );
    glBindTexture( GL_TEXTURE_3D, lookup_tex );

    vec2 dispatch_size, gws, lws;
    set_workgroup_size( gws, lws, dispatch_size, size );

    glDispatchCompute( dispatch_size.x, dispatch_size.y, 1 );

    glMemoryBarrier( GL_SHADER_IMAGE_ACCESS_BARRIER_BIT );
  }

  void display_texture( const vec2& size, GLuint src_tex, int src_level )
  {
    glViewport( 0, 0, size.x, size.y );

    glDisable( GL_DEPTH_TEST );

    glUseProgram( display_shader );
    glBindFramebuffer( GL_FRAMEBUFFER, 0 );

    glBindImageTexture( 0, src_tex, src_level, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8 );

    glUniform1f( 0, src_level );

    glBindVertexArray( ss_quad );
    glDrawElements( GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0 );

    glMemoryBarrier( GL_SHADER_IMAGE_ACCESS_BARRIER_BIT );
  }

public:
  post_process()
  {
    main_fbo = 0;
    color_tex = 0;
    pingpong_tex = 0;
    attribute_tex = 0;
    motion_tex = 0;
    downsample_shader = 0;
    gauss_shader = 0;
    radial_shader = 0;
    display_shader = 0;
    ss_quad = 0;
    screensize = vec2( 0 );
  }

  void destroy()
  {
    /**
    delete all gl resouces
    /**/

    auto delete_fbo = []( GLuint* f )
    {
      assert( f );
      if( *f )
      {
        glDeleteFramebuffers( 1, f );
        *f = 0;
      }
    };

    auto delete_texture = []( GLuint* f )
    {
      assert( f );
      if( *f )
      {
        glDeleteTextures( 1, f );
        *f = 0;
      }
    };

    auto delete_shader = []( GLuint* f )
    {
      assert( f );
      if( *f )
      {
        glDeleteProgram( *f );
        *f = 0;
      }
    };

    delete_fbo( &main_fbo );

    delete_texture( &color_tex );
    delete_texture( &pingpong_tex );
    delete_texture( &attribute_tex );
    delete_texture( &motion_tex );

    delete_shader( &downsample_shader );
    delete_shader( &gauss_shader );
    delete_shader( &radial_shader );
    delete_shader( &display_shader );
  }

  void set_up( int w, int h )
  {
    /**
    init textures, fbos
    /**/
    screensize = vec2( w, h );

    frm.load_shader( downsample_shader, GL_COMPUTE_SHADER, "../shaders/downsample/downsample.cs" );
    frm.load_shader( gauss_shader, GL_COMPUTE_SHADER, "../shaders/blur/gauss.cs" );
    frm.load_shader( radial_shader, GL_COMPUTE_SHADER, "../shaders/blur/radial.cs" );

    frm.load_shader( display_shader, GL_VERTEX_SHADER, "../shaders/display/display.vs" );
    frm.load_shader( display_shader, GL_FRAGMENT_SHADER, "../shaders/display/display.ps" );

    ss_quad = frm.create_quad( vec3( -1, -1, 0 ), vec3( 1, -1, 0 ), vec3( -1, 1, 0 ), vec3( 1, 1, 0 ) );

    glGenTextures( 1, &color_tex );
    glGenTextures( 1, &pingpong_tex );
    glGenTextures( 1, &attribute_tex );
    glGenTextures( 1, &motion_tex );
    glGenTextures( 1, &lookup_tex );

    vec4 lookup_buf[16][16][16];

    for( int x = 0; x < 16; ++x )
    for( int y = 0; y < 16; ++y )
    for( int z = 0; z < 16; ++z )
      lookup_buf[z][y][x] = vec4( x, y, z, 16 ) / 16.0f;

    glBindTexture( GL_TEXTURE_3D, lookup_tex );
    glTexParameteri( GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    glTexImage3D( GL_TEXTURE_3D, 0, GL_RGBA8, 16, 16, 16, 0, GL_RGBA, GL_FLOAT, lookup_buf );

    glBindTexture( GL_TEXTURE_2D, color_tex );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0 );
    glGenerateMipmap( GL_TEXTURE_2D ); //allocate mip levels

    glBindTexture( GL_TEXTURE_2D, pingpong_tex );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0 );
    glGenerateMipmap( GL_TEXTURE_2D ); //allocate mip levels

    glBindTexture( GL_TEXTURE_2D, attribute_tex );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0 );

    glBindTexture( GL_TEXTURE_2D, motion_tex );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RG16F, w, h, 0, GL_RG, GL_FLOAT, 0 );

    //RGB8 color
    //R8 shadow amount, G8 chromatic aberration amount, B8 glow amount, A8 blur amount
    //RG16F motion vectors
    glGenFramebuffers( 1, &main_fbo );
    glBindFramebuffer( GL_FRAMEBUFFER, main_fbo );
    GLenum bufs[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
    glDrawBuffers( 3, bufs );

    glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_tex, 0 );
    glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, attribute_tex, 0 );
    glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, motion_tex, 0 );

    frm.check_fbo_status();

    glBindFramebuffer( GL_FRAMEBUFFER, 0 );
  }

  void start_recording()
  {
    /**
    bind fbo
    /**/

    glBindFramebuffer( GL_FRAMEBUFFER, main_fbo );
  }

  void end_recording()
  {
    /**
    unbind fbo
    /**/

    glBindFramebuffer( GL_FRAMEBUFFER, 0 );
  }

  void start_post_process()
  {
    /**
    do the post process
    /**/

    gen_mipmaps( color_tex, GL_RGBA8, screensize, 2 );

    radial_blur( vec2( 0.5 ), 0.9, 0.2, 0.4, 0.8, vec2( screensize ) * 0.5f, color_tex, 1, pingpong_tex, 1 );

    gauss_blur( vec2( 1, 0 ), screensize * 0.5f, pingpong_tex, 1, color_tex, 1 );
    gauss_blur( vec2( 0, 1 ), screensize * 0.5f, color_tex, 1, pingpong_tex, 1 );

    display_texture( screensize, pingpong_tex, 1 );
  }
};