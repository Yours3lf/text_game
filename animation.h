#pragma once

#include "mymath/mymath.h"
#include "font.h"
#include "transition.h"
#include <string>

using namespace std;
using namespace mymath;

class animation
{
  public: 
    enum anim : unsigned
    {
      NONE = (0), 
      ALPHA = (1 << 0),
      POSITION = (1 << 1),
      ROTATION = (1 << 2),
      SCALE = (1 << 3),
      SIZE = (1 << 4)
    };

  private:
    font_inst* f;
    wstring text;
    vec4 base_color, base_highlight_color;
    mat4 base_transformation;
    float base_line_height;
    int base_font_size;
    unsigned active_anim;
    float duration;
    float time;
    int repeat_amount;
    int repeat_counter;
    vec3 start_pos, end_pos;
    float start_rotation, end_rotation;
    int start_size, end_size;
    vec3 start_scale, end_scale;
    bool is_playing, is_looping, do_display;
    float play_direction;
    vector<animation*> chain_list;
    transition::func trans[32];

  public:

    void update( float dt )
    {
      if( !is_playing && !do_display ) return;

      mat4 m = base_transformation;
      vec4 c = base_color;
      vec4 h = base_highlight_color;
      float s = base_font_size;
      float l = base_line_height;
      vec3 p = vec3(0);
      vec3 sc = vec3(1);
      float r = 0;
      float t;

      if( is_looping )
      {
        if( time + dt * play_direction > duration )
        {
          for( auto& c : chain_list )
          {
            c->play();
          }

          float rest = time + dt * play_direction - duration;
          dt = rest;
          play_direction *= -1;
        }
        else if( time + dt * play_direction < 0 )
        {
          float rest = fabs(time + dt * play_direction);
          dt = rest;
          play_direction *= -1;
        }

        time += dt * play_direction;
      }
      else
      {
        if( time + dt * play_direction >= duration )
        {
          repeat_counter++;

          float rest = time + dt * play_direction - duration;
          dt = rest;

          for( auto& c : chain_list )
          {
            c->play();
          }

          if( repeat_counter < repeat_amount )
          {
            play_direction *= -1;
            time += dt * play_direction;
          }
          else
          {
            is_playing = false;
          }
        }
        else if( time + dt * play_direction < 0 )
        {
          repeat_counter++;

          float rest = fabs(time + dt * play_direction);
          dt = rest;

          if( repeat_counter < repeat_amount )
          {
            play_direction *= -1;
            time += dt * play_direction;
          }
          else
          {
            is_playing = false;
          }
        }
        else
        {
          time += dt * play_direction;
        }
      }

      t = time / duration;

      if( active_anim & ALPHA )
      {
        if( !trans[0] )
          trans[0] = transition::linear;

        float fxt = trans[0](t);

        c.w = fxt;
      }

      if( active_anim & POSITION )
      {
        if( !trans[1] )
          trans[1] = transition::linear;

        float fxt = trans[1](t);

        vec3 delta_pos = end_pos - start_pos;
        p = start_pos + delta_pos * fxt;
      }

      if( active_anim & ROTATION )
      {
        if( !trans[2] )
          trans[2] = transition::linear;

        float fxt = trans[2](t);

        float delta_rot = end_rotation - start_rotation;
        r = start_rotation + delta_rot * fxt;
      }

      if( active_anim & SCALE )
      {
        if( !trans[3] )
          trans[3] = transition::linear;

        float fxt = trans[3](t);

        vec3 delta_scale = end_scale - start_scale;
        sc = start_scale + delta_scale * fxt;
      }

      if( active_anim & SIZE )
      {
        if( !trans[4] )
          trans[4] = transition::linear;

        float fxt = trans[4](t);

        int delta_size = end_size - start_size;
        s = start_size + delta_size * fxt;
      }

      float filter_value = 0;
      {
        float ft = t * 2 - 1;
        if( ft > 0 )
        {
          float fxt = transition::circular_in( ft );
          filter_value = 1 - fxt;
        }
        else
        {
          float fxt = transition::circular_out( 1 + ft );
          filter_value = fxt;
        }
      }

      if( !equal(sc, vec3(1)) )
        filter_value = 1;

      m = create_translation( p ) * create_rotation( r, vec3( 0, 0, 1 ) ) * create_scale( sc );
      font::get().set_size( *f, s );
      font::get().add_to_render_list( text, *f, c, m, h, l, filter_value );
    }

    animation()
    {
      f = 0;
      base_color = vec4(1);
      base_highlight_color = vec4(0);
      base_transformation = mat4::identity;
      base_line_height = 1;
      base_font_size = 20;
      active_anim = NONE;
      time = 0;
      duration = 0;
      is_playing = false;
      is_looping = false;
      do_display = false;
      text = L"";
      start_pos = vec3(0);
      end_pos = vec3(0);
      start_rotation = 0;
      end_rotation = 0;
      start_scale = vec3(1);
      end_scale = vec3(1);
      repeat_amount = 1;
      repeat_counter = 0;
      play_direction = 1;
      start_size = 20;
      end_size = 20;

      for( int c = 0; c < 32; ++c )
      {
        trans[c] = 0;
      }
    }

    void chain_animation( animation* a )
    {
      assert( a );
      chain_list.push_back( a );
    }

    void play()
    {
      is_playing = true;
      do_display = true;
    }

    void pause()
    {
      is_playing = false;
    }

    void stop()
    {
      time = 0;
      repeat_counter = 0;
      update(0);
      is_playing = false;
      do_display = false;
    }

    void set_repeat_amount( int r )
    {
      repeat_amount = r;
    }

    void set_start_size( int s )
    {
      start_size = s;
    }

    void set_end_size( int s )
    {
      end_size = s;
    }

    void set_start_scale( vec3 s )
    {
      start_scale = s;
    }

    void set_end_scale( vec3 s )
    {
      end_scale = s;
    }

    void set_start_pos( vec3 p )
    {
      start_pos = p;
    }

    void set_end_pos( vec3 p )
    {
      end_pos = p;
    }
    
    //radians
    void set_start_rotation( float r )
    {
      start_rotation = r;
    }

    //radians
    void set_end_rotation( float r )
    {
      end_rotation = r;
    }

    void set_transition( anim a, transition::func t )
    {
      assert( a / 2 >= 0 && a / 2 < 32 );
      trans[a / 2] = t;
    }

    void set_loop( bool l )
    {
      is_looping = l;
    }

    void set_duration( float t )
    {
      duration = t;
    }

    void set_text( wstring s )
    {
      text = s;
    }

    void set_font_size( int s )
    {
      base_font_size = s;
    }

    void turn_on_animation( anim a )
    {
      active_anim |= (unsigned)a;
    }

    void turn_off_animation( anim a )
    {
      active_anim &= ~(unsigned)a;
    }

    void set_font( font_inst* ff )
    {
      f = ff;
    }

    void set_color( vec4 color )
    {
      base_color = color;
    }

    void set_highlight_color( vec4 color )
    {
      base_highlight_color = color;
    }

    void set_transformation( mat4 m )
    {
      base_transformation = m;
    }

    void set_line_height( float h )
    {
      base_line_height = h;
    }
};