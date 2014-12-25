#include "util.h"

#include "browser.h"
#include "font.h"
#include "animation.h"
#include "transition.h"

#include <sstream>
#include <string>
#include <iostream>
#include <locale>
#include <codecvt>

namespace js
{
  void bindings_complete( const browser_instance& w )
  {
    std::wstringstream ws;
    ws << "bindings_complete();";

    browser::get().execute_javascript( w, ws.str() );
  }
}

void browser::onTitleChanged( Berkelium::Window* win,
                              Berkelium::WideString title )
{
  std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> conv;
  std::wstring str( title.mData, title.mLength );
  frm.set_title( conv.to_bytes(str) );
}

template< class t >
void visualize( t f, vec3 color = vec3(1) )
{
  glUseProgram(0);

  glMatrixMode( GL_MODELVIEW );
  glLoadIdentity();
  glMatrixMode( GL_PROJECTION );
  mat4 m = ortographic( 0.0f, 100.0f, 0.0f, 100.0f, 0.0f, 1.0f );
  glLoadMatrixf(&m[0][0]);
  glColor3fv( &color.x );

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_LINE_SMOOTH);
  glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

  glBegin( GL_LINE_STRIP );
  for( int c = 0; c <= 500; ++c )
  {
    float val = c / 500.0f;

    float x_val = val;
    float y_val = f(val);
      
    glVertex2f( x_val * 90 + 5, y_val * 50 + 25 );
  }
  glEnd();
}

int main( int argc, char** args )
{
  shape::set_up_intersection();

  frm.init(res);
  frm.set_vsync( true );

  glDepthFunc( GL_LEQUAL );
  glFrontFace( GL_CCW );
  glEnable( GL_CULL_FACE );
  glClearColor( 0.0f, 0.0f, 0.0f, 0.0f ); //sky color
  glClearDepth( 1.0f );

  glViewport( 0, 0, res.x, res.y );

  frm.get_opengl_error();

  GLuint ss_quad = frm.create_quad( vec3(-1, -1, 0), vec3(1, -1, 0), vec3(-1, 1, 0), vec3(1, 1, 0) );

  string path = frm.get_app_path();
  
  browser::get().init( L"../resources/berkelium/win32" );
  browser::get().create( b, res );
  browser::get().navigate( b, string("file:///") + path + "resources/ui/ui.html" );

  //load browser shader
  GLuint browser_shader = 0;
  frm.load_shader( browser_shader, GL_VERTEX_SHADER, "../shaders/browser/browser.vs" );
  frm.load_shader( browser_shader, GL_FRAGMENT_SHADER, "../shaders/browser/browser.ps" );

  frm.load_shader( font::get().get_shader(), GL_VERTEX_SHADER, "../shaders/font/font.vs" );
  frm.load_shader( font::get().get_shader(), GL_FRAGMENT_SHADER, "../shaders/font/font.ps" );

  font_inst font_instance;
  font::get().resize( res );
  font::get().load_font( "../resources/font.ttf", font_instance, 20 );

  animation anim, anim2;
  
  anim.set_font( &font_instance );
  anim.set_duration( 1 );
  anim.set_loop( false );
  anim.set_text( L"hello world" );
  anim.set_font_size( 20 );
  anim.set_duration( 1 );
  anim.set_transition( animation::ALPHA, transition::quadratic_inout );
  anim.set_transition( animation::POSITION, transition::quadratic_inout );
  anim.turn_on_animation( animation::ALPHA );
  anim.turn_on_animation( animation::POSITION );
  anim.turn_on_animation( animation::ROTATION );
  anim.set_start_rotation( radians( 90 ) );
  anim.set_end_rotation( radians( 0 ) );
  anim.set_start_pos( vec3( 0, 0, 0 ) );
  anim.set_end_pos( vec3( 20, 0, 0 ) );
  anim.chain_animation( &anim2 );
  anim.play();

  anim2.set_font( &font_instance );
  anim2.set_duration( 1 );
  anim2.set_loop( false );
  anim2.set_text( L"chained animation" );
  anim2.set_font_size( 19 );
  anim2.set_duration( 1 );
  anim2.set_transition( animation::ALPHA, transition::quadratic_inout );
  anim2.set_transition( animation::POSITION, transition::quadratic_inout );
  anim2.turn_on_animation( animation::ALPHA );
  anim2.turn_on_animation( animation::POSITION );
  anim2.turn_on_animation( animation::ROTATION );
  anim2.turn_on_animation( animation::SCALE );
  anim2.turn_on_animation( animation::SIZE );
  anim2.set_start_rotation( radians( 90 ) );
  anim2.set_end_rotation( radians( 0 ) );
  anim2.set_start_pos( vec3( 120, 0, 0 ) );
  anim2.set_end_pos( vec3( 140, 0, 0 ) );
  anim2.set_start_scale( vec3( 1 ) );
  anim2.set_end_scale( vec3( 1.1 ) );
  anim2.set_start_size( 20 );
  anim2.set_end_size( 72 );
  anim2.set_repeat_amount( 3 );

  sf::Clock global_timer;

  frm.display(
  [&]()
  {
    frm.handle_events( [&]( const sf::Event & ev )
    {
      switch( ev.type )
      {
        case sf::Event::MouseMoved:
          {
            vec2 mpos( ev.mouseMove.x / float( res.x ), ev.mouseMove.y / float( res.y ) );

            browser::get().mouse_moved( b, mpos );

            break;
          }
        case sf::Event::KeyPressed:
          {
            /*if( ev.key.code == sf::Keyboard::A )
            {
              cam.rotate_y( radians( cam_rotation_amount ) );
            }*/

            break;
          }
        case sf::Event::TextEntered:
        {
          wchar_t txt[2];
          txt[0] = ev.text.unicode;
          txt[1] = '\0';
          browser::get().text_entered( b, txt );

          break;
        }
        case sf::Event::MouseButtonPressed:
        {
          if( ev.mouseButton.button == sf::Mouse::Left )
          {
            browser::get().mouse_button_event( b, sf::Mouse::Left, true );
          }
          else
          {
            browser::get().mouse_button_event( b, sf::Mouse::Right, true );
          }

          break;
        }
        case sf::Event::MouseButtonReleased:
        {
          if( ev.mouseButton.button == sf::Mouse::Left )
          {
            browser::get().mouse_button_event( b, sf::Mouse::Left, false );
          }
          else
          {
            browser::get().mouse_button_event( b, sf::Mouse::Right, false );
          }

          break;
        }
        case sf::Event::MouseWheelMoved:
        {
          browser::get().mouse_wheel_moved( b, ev.mouseWheel.delta * 100.0f );

          break;
        }
        case sf::Event::Resized:
        {
          res = uvec2( ev.size.width, ev.size.height );

          browser::get().resize( b, res );
          font::get().resize( res );

          break;
        }
        default:
          break;
      }
    } );

    browser::get().update();

    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    float dt = global_timer.getElapsedTime().asMilliseconds() * 0.001f;

    anim.update( dt );
    anim2.update( dt );
    cout << endl;

    font::get().render();

    //-----------------------------
    //render the UI
    //-----------------------------

    /**

    glDisable( GL_DEPTH_TEST );
    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glUseProgram( browser_shader );

    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_2D, b.browser_texture );
    
    glBindVertexArray( ss_quad );
    glDrawElements( GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0 );

    glDisable( GL_BLEND );
    glEnable( GL_DEPTH_TEST );

    /**/

    global_timer.restart();
  } );

  browser::get().destroy( b );
  browser::get().shutdown();

  return 0;
}
