#include "font.h"

#include <fstream>

#include "ft2build.h"
#include FT_FREETYPE_H

#define MAX_TEX_SIZE 8192
#define MIN_TEX_SIZE 128

#define FONT_VERTEX 0
#define FONT_TEXCOORD 1
#define FONT_VERTSCALEBIAS 2
#define FONT_TEXSCALEBIAS 3
#define FONT_COLOR 4
#define FONT_FACE 5
#define FONT_TRANSFORM 6
#define FONT_FILTER 7

wchar_t buf[2] = { -1, L'\0' };
std::wstring cachestring = std::wstring( buf ) + L" 0123456789a�bcde�fghi�jklmno���pqrstu���vwxyzA�BCDE�FGHI�JKLMNO���PQRSTU���VWXYZ+!%/=()|$[]<>#&@{},.~-?:_;*`^'\"";

struct glyph
{
  float offset_x;
  float offset_y;
  float w;
  float h;
  float texcoords[4];
  FT_Glyph_Metrics metrics;
  FT_Vector advance;
  FT_UInt glyphid;
  unsigned int cache_index;
};

library::library() : the_library( 0 ), tex( 0 ), texsampler_point( 0 ), texsampler_linear( 0 ), vao( 0 ), the_shader( 0 ), is_set_up( false )
{
  for( int c = 0; c < FONT_LIB_VBO_SIZE; ++c )
    vbos[c] = 0;

  FT_Error error;
  error = FT_Init_FreeType( (FT_Library*)&the_library );

  if( error )
  {
    std::cerr << "Error initializing the freetype library." << std::endl;
  }
}

void library::destroy()
{
  glDeleteSamplers( 1, &texsampler_point );
  glDeleteSamplers( 1, &texsampler_linear );
  glDeleteTextures( 1, &tex );
  glDeleteVertexArrays( 1, &vao );
  glDeleteBuffers( FONT_LIB_VBO_SIZE, vbos );
  glDeleteProgram( the_shader );
}

library::~library()
{
  if( the_library )
  {
    FT_Error error;
    error = FT_Done_FreeType( (FT_Library)the_library );

    if( error )
    {
      std::cerr << "Error destroying the freetype library." << std::endl;
    }
  }
}

void library::delete_glyphs()
{
  texture_pen = mm::uvec2( 1 );
  texture_row_h = 0;
  texsize = mm::uvec2( 0 );

  font_data.clear();

  for( auto& c : instances )
  {
    ( *c->the_face->glyphs ).clear();
  }
}

void library::set_up()
{
  if( is_set_up ) return;

  texture_pen = mm::uvec2( 0 );
  texsize = mm::uvec2( 0 );

  glGenTextures( 1, &tex );

  glGenSamplers( 1, &texsampler_point );
  glGenSamplers( 1, &texsampler_linear );

  glSamplerParameteri( texsampler_point, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
  glSamplerParameteri( texsampler_point, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
  glSamplerParameteri( texsampler_point, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
  glSamplerParameteri( texsampler_point, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

  glSamplerParameteri( texsampler_linear, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
  glSamplerParameteri( texsampler_linear, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
  glSamplerParameteri( texsampler_linear, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
  glSamplerParameteri( texsampler_linear, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

  std::vector<unsigned> faces;
  std::vector<float> vertices;
  std::vector<float> texcoords;
  faces.resize( 2 * 3 );
  vertices.resize( 4 * 2 );
  texcoords.resize( 4 * 2 );

  faces[0 * 3 + 0] = 2;
  faces[0 * 3 + 1] = 1;
  faces[0 * 3 + 2] = 0;

  faces[1 * 3 + 0] = 0;
  faces[1 * 3 + 1] = 3;
  faces[1 * 3 + 2] = 2;

  vertices[0 * 2 + 0] = 0;
  vertices[0 * 2 + 1] = 0;

  vertices[1 * 2 + 0] = 0;
  vertices[1 * 2 + 1] = 1;

  vertices[2 * 2 + 0] = 1;
  vertices[2 * 2 + 1] = 1;

  vertices[3 * 2 + 0] = 1;
  vertices[3 * 2 + 1] = 0;

  texcoords[0 * 2 + 0] = 0;
  texcoords[0 * 2 + 1] = 0;

  texcoords[1 * 2 + 0] = 0;
  texcoords[1 * 2 + 1] = 1;

  texcoords[2 * 2 + 0] = 1;
  texcoords[2 * 2 + 1] = 1;

  texcoords[3 * 2 + 0] = 1;
  texcoords[3 * 2 + 1] = 0;

  glGenVertexArrays( 1, &vao );
  glBindVertexArray( vao );

  glGenBuffers( 1, &vbos[FONT_VERTEX] );
  glBindBuffer( GL_ARRAY_BUFFER, vbos[FONT_VERTEX] );
  glBufferData( GL_ARRAY_BUFFER, sizeof(float)* 2 * vertices.size(), &vertices[0], GL_STATIC_DRAW );
  glEnableVertexAttribArray( FONT_VERTEX );
  glVertexAttribPointer( FONT_VERTEX, 2, GL_FLOAT, false, 0, 0 );

  glGenBuffers( 1, &vbos[FONT_TEXCOORD] );
  glBindBuffer( GL_ARRAY_BUFFER, vbos[FONT_TEXCOORD] );
  glBufferData( GL_ARRAY_BUFFER, sizeof(float)* 2 * texcoords.size(), &texcoords[0], GL_STATIC_DRAW );
  glEnableVertexAttribArray( FONT_TEXCOORD );
  glVertexAttribPointer( FONT_TEXCOORD, 2, GL_FLOAT, false, 0, 0 );

  glGenBuffers( 1, &vbos[FONT_VERTSCALEBIAS] );
  glBindBuffer( GL_ARRAY_BUFFER, vbos[FONT_VERTSCALEBIAS] );
  glEnableVertexAttribArray( FONT_VERTSCALEBIAS );
  glVertexAttribPointer( FONT_VERTSCALEBIAS, 4, GL_FLOAT, false, sizeof( mm::vec4 ), 0 );
  glVertexAttribDivisor( FONT_VERTSCALEBIAS, 1 );

  glGenBuffers( 1, &vbos[FONT_TEXSCALEBIAS] );
  glBindBuffer( GL_ARRAY_BUFFER, vbos[FONT_TEXSCALEBIAS] );
  glEnableVertexAttribArray( FONT_TEXSCALEBIAS );
  glVertexAttribPointer( FONT_TEXSCALEBIAS, 4, GL_FLOAT, false, sizeof( mm::vec4 ), 0 );
  glVertexAttribDivisor( FONT_TEXSCALEBIAS, 1 );

  glGenBuffers( 1, &vbos[FONT_COLOR] );
  glBindBuffer( GL_ARRAY_BUFFER, vbos[FONT_COLOR] );
  glEnableVertexAttribArray( FONT_COLOR );
  glVertexAttribPointer( FONT_COLOR, 4, GL_FLOAT, false, sizeof( mm::vec4 ), 0 );
  glVertexAttribDivisor( FONT_COLOR, 1 );

  glGenBuffers( 1, &vbos[FONT_FACE] );
  glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, vbos[FONT_FACE] );
  glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned)* 3 * faces.size(), &faces[0], GL_STATIC_DRAW );
  glGenBuffers( 1, &vbos[FONT_TRANSFORM] );
  glBindBuffer( GL_ARRAY_BUFFER, vbos[FONT_TRANSFORM] );

  for( int c = 0; c < 4; ++c )
  {
    glEnableVertexAttribArray( FONT_TRANSFORM + c );
    glVertexAttribPointer( FONT_TRANSFORM + c, 4, GL_FLOAT, false, sizeof( mm::mat4 ), ( (char*)0 ) + sizeof( mm::vec4 ) * c );
    glVertexAttribDivisor( FONT_TRANSFORM + c, 1 );
  }

  glGenBuffers( 1, &vbos[FONT_FILTER] );
  glBindBuffer( GL_ARRAY_BUFFER, vbos[FONT_FILTER] );
  glEnableVertexAttribArray( FONT_FILTER + 3 );
  glVertexAttribPointer( FONT_FILTER + 3, 1, GL_FLOAT, false, sizeof( float ), 0 );
  glVertexAttribDivisor( FONT_FILTER + 3, 1 );

  glBindVertexArray( 0 );

  is_set_up = true;
}

bool library::expand_tex()
{
  glBindTexture( GL_TEXTURE_RECTANGLE, tex );

  if( texsize.x == 0 || texsize.y == 0 )
  {
    texsize.x = MAX_TEX_SIZE;
    texsize.y = MIN_TEX_SIZE;

    GLubyte* buf = new GLubyte[texsize.x * texsize.y];
    memset( buf, 0, texsize.x * texsize.y * sizeof( GLubyte ) );

    glTexParameteri( GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE );
    glTexImage2D( GL_TEXTURE_RECTANGLE, 0, GL_R8, texsize.x, texsize.y, 0, GL_RED, GL_UNSIGNED_BYTE, buf );

    texture_pen.x = 1;
    texture_pen.y = 1;
    texture_row_h = 0;

    delete[] buf;
  }
  else
  {
    GLubyte* buf = new GLubyte[texsize.x * texsize.y];

    glGetTexImage( GL_TEXTURE_RECTANGLE, 0, GL_RED, GL_UNSIGNED_BYTE, buf );

    int tmp_height = texsize.y;
    texsize.y += MIN_TEX_SIZE;

    if( texsize.y > MAX_TEX_SIZE ) //can't expand tex further
    {
      return false;
    }

    GLubyte* tmpbuf = new GLubyte[texsize.x * texsize.y];
    memset( tmpbuf, 0, texsize.x * texsize.y * sizeof( GLubyte ) );

    glTexImage2D( GL_TEXTURE_RECTANGLE, 0, GL_R8, texsize.x, texsize.y, 0, GL_RED, GL_UNSIGNED_BYTE, tmpbuf );
    glTexSubImage2D( GL_TEXTURE_RECTANGLE, 0, 0, 0, texsize.x, tmp_height, GL_RED, GL_UNSIGNED_BYTE, buf );

    delete[] buf;
    delete[] tmpbuf;
  }

  return true;
}

font_inst::face::face() : size( 0 ), the_face( 0 ), glyphs( 0 )
{
}

font_inst::face::face( const std::string& filename, unsigned int index )
{
  FT_Error error;
  error = FT_New_Face( (FT_Library)library::get().get_library(), filename.c_str(), index, (FT_Face*)&the_face );

  upos = 0;
  uthick = 0;

  FT_Matrix matrix = { (int)( ( 1.0 / 64.0f ) * 0x10000L ),
                       (int)( ( 0.0 ) * 0x10000L ),
                       (int)( ( 0.0 ) * 0x10000L ),
                       (int)( ( 1.0 ) * 0x10000L ) };
  FT_Select_Charmap( *(FT_Face*)&the_face, FT_ENCODING_UNICODE );
  FT_Set_Transform( *(FT_Face*)&the_face, &matrix, NULL );

  glyphs = new std::map< unsigned int, std::map<uint32_t, glyph> >();

  if( error )
  {
    std::cerr << "Error loading font face: " << filename << std::endl;
    the_face = 0;
  }
}

font_inst::face::~face()
{
  //TODO invalidate glyphs in the library, and its tex
  FT_Done_Face( (FT_Face)the_face );
  delete glyphs;
}

void font_inst::face::set_size( unsigned int val )
{
  if( the_face )
  {
    size = val;

    FT_Set_Char_Size( (FT_Face)the_face, size * 100.0f * 64.0f, 0.0f, 72 * 64.0f, 72 );
    asc = ( ( (FT_Face)the_face )->size->metrics.ascender / 64.0f ) / 100.0f;
    desc = ( ( (FT_Face)the_face )->size->metrics.descender / 64.0f ) / 100.0f;
    h = ( ( (FT_Face)the_face )->size->metrics.height / 64.0f ) / 100.0f;
    gap = h - asc + desc;

    upos = ( (FT_Face)the_face )->underline_position / ( 64.0f*64.0f ) * size;
    upos = std::round( upos );

    if( upos > -2 )
    {
      upos = -2;
    }

    uthick = ( (FT_Face)the_face )->underline_thickness / ( 64.0f*64.0f ) * size;
    uthick = std::round( uthick );

    if( uthick < 1 )
    {
      uthick = 1;
    }

    FT_Set_Char_Size( (FT_Face)the_face, size * 64.0f, 0.0f, 72 * 64.0f, 72 );
  }
}

bool font_inst::face::load_glyph( unsigned int val )
{
  if( ( *glyphs )[size].count( val ) == 0 && the_face )
  {
    FT_Error error;

    FT_GlyphSlot theglyph = FT_GlyphSlot();

    if( val != wchar_t( -1 ) )
    {
      error = FT_Load_Char( (FT_Face)the_face, (const FT_UInt)val, FT_LOAD_RENDER | FT_LOAD_FORCE_AUTOHINT );

      if( error )
      {
        std::cerr << "Error loading character: " << (wchar_t)val << std::endl;
      }

      theglyph = ( (FT_Face)the_face )->glyph;
    }
    else
    {
      theglyph = new FT_GlyphSlotRec_();
      theglyph->advance.x = 0;
      theglyph->advance.y = 0;
      static unsigned char data[4 * 4 * 3] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                                          -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                                          -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                                          -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };
      theglyph->bitmap.buffer = data;
      theglyph->bitmap.rows = 4;
      theglyph->bitmap.width = 4;
      theglyph->bitmap_left = 0;
      theglyph->bitmap_top = 0;
    }

    auto texsize = library::get().get_texsize();
    auto& texpen = library::get().get_texture_pen();
    auto& texrowh = library::get().get_tex_row_h();

    if( texsize.x == 0 || texsize.y == 0 )
    {
      library::get().expand_tex();
    }

    FT_Bitmap* bitmap = &theglyph->bitmap;

    if( texpen.x + bitmap->width + 1 > (int)texsize.x )
    {
      texpen.y += texrowh + 1;
      texpen.x = 1;
      texrowh = 0;
    }

    if( texpen.y + bitmap->rows + 1 > (int)texsize.y )
    {
      if( !library::get().expand_tex() )
      {
        //tex expansion unsuccessful
        return false;
      }
    }

    GLubyte* data;
    int glyph_size = bitmap->width * bitmap->rows;
    data = new GLubyte[glyph_size];

    int c = 0;

    for( int y = 0; y < bitmap->rows; y++ )
    {
      for( int x = 0; x < bitmap->width; x++ )
      {
        data[x + ( bitmap->rows - 1 - y ) * bitmap->width] = bitmap->buffer[c++];
      }
    }

    GLint uplast;
    glGetIntegerv( GL_UNPACK_ALIGNMENT, &uplast );

    if( uplast != 1 )
    {
      glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
    }

    glBindTexture( GL_TEXTURE_RECTANGLE, library::get().get_tex() );
    glTexSubImage2D( GL_TEXTURE_RECTANGLE, 0, texpen.x, texpen.y, bitmap->width, bitmap->rows, GL_RED, GL_UNSIGNED_BYTE, data );

    delete[] data;

    if( uplast != 1 )
    {
      glPixelStorei( GL_UNPACK_ALIGNMENT, uplast );
    }

    glyph* g = &( *glyphs )[size][val];

    g->glyphid = FT_Get_Char_Index( (FT_Face)the_face, (const FT_ULong)val );

    g->offset_x = (float)theglyph->bitmap_left;
    g->offset_y = (float)theglyph->bitmap_top;
    g->w = (float)bitmap->width;
    g->h = (float)bitmap->rows;

    if( val != wchar_t( -1 ) )
    {
      g->texcoords[0] = (float)texpen.x - 0.5f;
      g->texcoords[1] = (float)texpen.y - 0.5f;
      g->texcoords[2] = (float)texpen.x + (float)bitmap->width + 0.5f;
      g->texcoords[3] = (float)texpen.y + (float)bitmap->rows + 0.5f;
    }
    else
    {
      g->texcoords[0] = (float)texpen.x;
      g->texcoords[1] = (float)texpen.y;
      g->texcoords[2] = (float)texpen.x + (float)bitmap->width;
      g->texcoords[3] = (float)texpen.y + (float)bitmap->rows;
    }

    texpen.x += bitmap->width + 1;

    if( bitmap->rows > texrowh )
    {
      texrowh = bitmap->rows;
    }

    g->advance = theglyph->advance;
  }

  return true;
}

float font_inst::face::kerning( const uint32_t prev, const uint32_t next )
{
  if( the_face )
  {
    if( next && FT_HAS_KERNING( ( (FT_Face)the_face ) ) )
    {
      FT_Vector kern;
      FT_Get_Kerning( (FT_Face)the_face, prev, next, FT_KERNING_UNFITTED, &kern );
      return ( kern.x / ( 64.0f*64.0f ) );
    }
    else
    {
      return 0;
    }
  }
  else
  {
    return 0;
  }
}

float font_inst::face::advance( const uint32_t current )
{
  return ( ( *glyphs )[size][current].advance.x / 64.0f );
}

float font_inst::face::height()
{
  return h;
}

float font_inst::face::linegap()
{
  return gap;
}

float font_inst::face::ascender()
{
  return asc;
}

float font_inst::face::descender()
{
  return desc;
}

float font_inst::face::underline_position()
{
  return upos;
}

float font_inst::face::underline_thickness()
{
  return uthick;
}

glyph& font_inst::face::get_glyph( uint32_t i )
{
  return ( *glyphs )[size][i];
}

bool font_inst::face::has_glyph( uint32_t i )
{
  try
  {
    ( *glyphs ).at( size ).at( i );
    return true;
  }
  catch( ... )
  {
    return false;
  }
}

void font::set_size( font_inst& font_ptr, unsigned int s )
{
  font_ptr.the_face->set_size( s );

  std::for_each( cachestring.begin(), cachestring.end(),
                 [&]( wchar_t & c )
  {
    add_glyph( font_ptr, c );
  } );
}

void font::add_glyph( font_inst& font_ptr, uint32_t c, int counter )
{
  if( font_ptr.the_face->has_glyph( c ) )
    return;

  if( !font_ptr.the_face->load_glyph( c ) )
  {
    if( counter > 9 ) //at max 10 tries
    {
      exit( 1 ); //couldn't get enough memory or something... (extreme case) 
    }

    library::get().delete_glyphs();
    add_glyph( font_ptr, c, ++counter );
    return;
  }

  auto& g = font_ptr.the_face->get_glyph( c );

  g.cache_index = library::get().get_font_data_size();

  mm::vec2 vertbias = mm::vec2( g.offset_x - 0.5f, -0.5f - ( g.h - g.offset_y ) );
  mm::vec2 vertscale = mm::vec2( g.offset_x + g.w + 0.5f, 0.5f + g.h - ( g.h - g.offset_y ) ) - vertbias;

  //texcoords
  mm::vec2 texbias = mm::vec2( g.texcoords[0], g.texcoords[1] );
  mm::vec2 texscale = mm::vec2( g.texcoords[2], g.texcoords[3] ) - texbias;

  library::get().add_font_data( fontscalebias( vertscale, vertbias, texscale, texbias ) );
}

void font::load_font( const std::string& filename, font_inst& font_ptr, unsigned int size )
{
  std::cout << "-Loading: " << filename << std::endl;

  std::fstream f( filename.c_str(), std::ios::in );

  if( !f )
    std::cerr << "Couldn't open file: " << filename << std::endl;

  f.close();

  library::get().set_up();
  resize( screensize );

  //load directly from font
  font_ptr.the_face = new font_inst::face( filename, 0 );

  set_size( font_ptr, size );

  library::get().instances.push_back( &font_ptr );
}

void font::resize( const mm::uvec2& ss )
{
  screensize = ss;
  font_frame.set_ortographic( 0.0f, (float)ss.x, 0.0f, (float)ss.y, 0.0f, 1.0f );
}

static std::vector<mm::vec4> vertscalebias;
static std::vector<mm::vec4> texscalebias;
static std::vector<mm::vec4> fontcolor;
static std::vector<mm::mat4> transform;
static std::vector<float> filter;

//these special unicode characters denote the text markup begin/end
#define FONT_UNDERLINE_BEGIN L'\uE000'
#define FONT_UNDERLINE_END L'\uE001'
#define FONT_OVERLINE_BEGIN L'\uE002'
#define FONT_OVERLINE_END L'\uE003'
#define FONT_STRIKETHROUGH_BEGIN L'\uE004'
#define FONT_STRIKETHROUGH_END L'\uE005'
#define FONT_HIGHLIGHT_BEGIN L'\uE006'
#define FONT_HIGHLIGHT_END L'\uE007'

bool is_special( wchar_t c )
{
  return c == FONT_UNDERLINE_BEGIN ||
         c == FONT_UNDERLINE_END ||
         c == FONT_OVERLINE_BEGIN ||
         c == FONT_OVERLINE_END ||
         c == FONT_STRIKETHROUGH_BEGIN ||
         c == FONT_STRIKETHROUGH_END ||
         c == FONT_HIGHLIGHT_BEGIN ||
         c == FONT_HIGHLIGHT_END;
}

mm::vec2 font::add_to_render_list( const std::wstring& txt, font_inst& font_ptr, const mm::vec4& color, const mm::mat4& mat, const mm::vec4& highlight_color, float line_height, float f )
{
  static bool underline = false;
  static bool overline = false;
  static bool strikethrough = false;
  static bool highlight = false;

  float yy = 0;
  float xx = 0;

  float vert_advance = font_ptr.the_face->height() - font_ptr.the_face->linegap();
  vert_advance *= line_height;

  yy += vert_advance;

  for( int c = 0; c < int( txt.size() ); c++ )
  {
    char bla = txt[c];

    if( txt[c] == L'\n' )
    {
      yy += vert_advance;
      xx = 0;
    }

    if( c > 0 && txt[c] != L'\n' && !is_special( txt[c] ) )
    {
      xx += font_ptr.the_face->kerning( txt[c - 1], txt[c] );
    }

    if( txt[c] == FONT_UNDERLINE_BEGIN )
      underline = true;
    else if( txt[c] == FONT_UNDERLINE_END )
      underline = false;
    else if( txt[c] == FONT_OVERLINE_BEGIN )
      overline = true;
    else if( txt[c] == FONT_OVERLINE_END )
      overline = false;
    else if( txt[c] == FONT_STRIKETHROUGH_BEGIN )
      strikethrough = true;
    else if( txt[c] == FONT_STRIKETHROUGH_END )
      strikethrough = false;
    else if( txt[c] == FONT_HIGHLIGHT_BEGIN )
      highlight = true;
    else if( txt[c] == FONT_HIGHLIGHT_END )
      highlight = false;

    float finalx = xx;
    mm::vec3 pos = mm::vec3( finalx, (float)screensize.y - yy, 0 );

    float advancex = 0;

    int i = 0;
    for( i = c; i < txt.size(); ++i )
    {
      if( !is_special( txt[i] ) )
        break;
    }
    advancex = font_ptr.the_face->advance( txt[i] );

    if( highlight )
    {
      unsigned int datapos = font_ptr.the_face->get_glyph( wchar_t( -1 ) ).cache_index;
      fontscalebias& fsb = library::get().get_font_data( datapos );
      fontscalebias copy = fsb;

      //vert bias
      copy.vertscalebias.w = font_ptr.the_face->descender();

      //hori bias
      copy.vertscalebias.z = 0.0f;

      //hori scale
      copy.vertscalebias.x = advancex;

      //vert scale
      copy.vertscalebias.y = font_ptr.the_face->height() + font_ptr.the_face->linegap();

      vertscalebias.push_back( mm::vec4( copy.vertscalebias.xy, copy.vertscalebias.zw + pos.xy ) );
      texscalebias.push_back( copy.texscalebias );
      fontcolor.push_back( highlight_color );
      transform.push_back( mat );
      filter.push_back( f );
    }

    if( strikethrough )
    {
      unsigned int datapos = font_ptr.the_face->get_glyph( wchar_t( -1 ) ).cache_index;
      fontscalebias& fsb = library::get().get_font_data( datapos );
      fontscalebias copy = fsb;

      //vert bias
      copy.vertscalebias.w = font_ptr.the_face->ascender() * 0.33f;

      //hori bias
      copy.vertscalebias.z = 0;

      //hori scale
      copy.vertscalebias.x = advancex;

      //vert scale
      copy.vertscalebias.y = font_ptr.the_face->underline_thickness();

      vertscalebias.push_back( mm::vec4( copy.vertscalebias.xy, copy.vertscalebias.zw + pos.xy ) );
      texscalebias.push_back( copy.texscalebias );
      fontcolor.push_back( color );
      transform.push_back( mat );
      filter.push_back( f );
    }

    if( underline )
    {
      unsigned int datapos = font_ptr.the_face->get_glyph( wchar_t( -1 ) ).cache_index;
      fontscalebias& fsb = library::get().get_font_data( datapos );
      fontscalebias copy = fsb;

      //vert bias
      copy.vertscalebias.w = font_ptr.the_face->underline_position();

      //hori bias
      copy.vertscalebias.z = 0.0f;

      //hori scale
      copy.vertscalebias.x = advancex;

      //vert scale
      copy.vertscalebias.y = font_ptr.the_face->underline_thickness();

      vertscalebias.push_back( mm::vec4( copy.vertscalebias.xy, copy.vertscalebias.zw + pos.xy ) );
      texscalebias.push_back( copy.texscalebias );
      fontcolor.push_back( color );
      transform.push_back( mat );
      filter.push_back( f );
    }

    if( overline )
    {
      unsigned int datapos = font_ptr.the_face->get_glyph( wchar_t( -1 ) ).cache_index;
      fontscalebias& fsb = library::get().get_font_data( datapos );
      fontscalebias copy = fsb;

      //vert bias
      copy.vertscalebias.w = font_ptr.the_face->ascender();

      //hori bias
      copy.vertscalebias.z = 0.0f;

      //hori scale
      copy.vertscalebias.x = advancex;

      //vert scale
      copy.vertscalebias.y = font_ptr.the_face->underline_thickness();

      vertscalebias.push_back( mm::vec4( copy.vertscalebias.xy, copy.vertscalebias.zw + pos.xy ) );
      texscalebias.push_back( copy.texscalebias );
      fontcolor.push_back( color );
      transform.push_back( mat );
      filter.push_back( f );
    }

    if( c < txt.size() && txt[c] != L' ' && txt[c] != L'\n' && !is_special( txt[c] ) )
    {
      add_glyph( font_ptr, txt[c] );

      unsigned int datapos = font_ptr.the_face->get_glyph( txt[c] ).cache_index;
      auto thefsb = library::get().get_font_data( datapos );
      vertscalebias.push_back( mm::vec4( thefsb.vertscalebias.xy, thefsb.vertscalebias.zw + pos.xy ) );
      texscalebias.push_back( thefsb.texscalebias );
      fontcolor.push_back( color );
      transform.push_back( mat );
      filter.push_back( f );
    }

    if( !is_special( txt[c] ) )
      xx += font_ptr.the_face->advance( txt[c] );
  }

  yy -= vert_advance;

  return mm::vec2( xx, yy );
}

void font::render()
{
  glDisable( GL_CULL_FACE );
  glDisable( GL_DEPTH_TEST );
  glEnable( GL_BLEND );
  glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

  library::get().bind_shader();

  //mvp is now only the projection matrix
  mm::mat4 mat = font_frame.projection_matrix;
  glUniformMatrix4fv( 0, 1, false, &mat[0].x );

  glActiveTexture( GL_TEXTURE0 );
  library::get().bind_texture();

  library::get().bind_vao();

  library::get().update_scalebiascolor( FONT_VERTSCALEBIAS, vertscalebias );
  library::get().update_scalebiascolor( FONT_TEXSCALEBIAS, texscalebias );
  library::get().update_scalebiascolor( FONT_COLOR, fontcolor );
  library::get().update_scalebiascolor( FONT_TRANSFORM, transform );
  library::get().update_scalebiascolor( FONT_FILTER, filter );

  glDrawElementsInstanced( GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, vertscalebias.size() );

  glBindVertexArray( 0 );

  glUseProgram( 0 );

  glDisable( GL_BLEND );
  glEnable( GL_DEPTH_TEST );
  glEnable( GL_CULL_FACE );

  vertscalebias.clear();
  texscalebias.clear();
  fontcolor.clear();
  transform.clear();
  filter.clear();
}
