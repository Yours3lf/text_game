#version 430 core

layout(binding=0, rgba8) readonly uniform image2D src_tex;
layout(binding=1) writeonly uniform image2D dst_tex;

layout(binding=2) uniform sampler3D lookup_tex;

layout(local_size_x = 16, local_size_y = 16) in; //local workgroup size

layout(location=0) uniform vec2 center = vec2(0);
layout(location=1) uniform float density = 0.96;
layout(location=2) uniform float positive_weight = 0.05;
layout(location=3) uniform float negative_weight = 0.4;
layout(location=4) uniform float decay = 0.93;
const float num_samples = 32;

vec4 sample_bilinear( vec2 coord, vec2 size )
{
  ivec2 final_coord = ivec2(coord * size);
  vec4 s00 = imageLoad( src_tex, final_coord + ivec2(0, 0) );
  vec4 s01 = imageLoad( src_tex, final_coord + ivec2(1, 0) );
  vec4 s10 = imageLoad( src_tex, final_coord + ivec2(0, 1) );
  vec4 s11 = imageLoad( src_tex, final_coord + ivec2(1, 1) );

  float xval = fract( coord.x * size.x );
  float yval = fract( coord.y * size.y );

  return mix( mix( s00, s10, xval ), mix( s01, s11, xval ), yval );
}

vec3 dither( vec2 coord, vec3 color )
{
  const float dither[8][8] =
  {
    {1, 49, 13, 61, 4, 52, 16, 64},
    {33, 17, 45, 29, 36, 20, 48, 32},
    {9, 57, 5, 53, 12, 60, 8, 56},
    {41, 25, 37, 21, 44, 28, 40, 24},
    {3, 51, 15, 63, 2, 50, 14, 62},
    {35, 19, 47, 31, 34, 18, 46, 30},
    {11, 59, 7, 55, 10, 58, 6, 54},
    {43, 27, 39, 23, 42, 26, 38, 22}
  };

  if( dot(color, color) < 0.001 )
    return vec3(0);

  ivec2 icoord = ivec2(mod( coord, vec2(8) ));

  vec3 dither_color = vec3(dither[icoord.x][icoord.y] / 65.0) / 16.0;
  vec3 oldpixel = color + dither_color;
  return texture( lookup_tex, oldpixel ).xyz;
}

void main()
{
  ivec2 global_id = ivec2( gl_GlobalInvocationID.xy );
  ivec2 global_size = imageSize( dst_tex ).xy;
  ivec2 src_size = imageSize( src_tex ).xy;
  vec2 orig_texcoord = vec2(global_id) / vec2(global_size);

  vec2 delta_texcoord = orig_texcoord - center;

  delta_texcoord = delta_texcoord / num_samples * density;

  vec4 result = vec4( 0, 0, 0, 1 );
  float illumination_decay = 1;
  vec4 positive_ray_result = vec4(0);
  vec4 negative_ray_result = vec4(0);
  float positive_counter = 0;
  float negative_counter = 0;

  vec2 texcoord = orig_texcoord;

  for( float c = 0; c < num_samples; ++c )
  {
    texcoord -= delta_texcoord;

    vec4 the_sample = sample_bilinear( texcoord, src_size );
    the_sample.xyz *= illumination_decay;

    float pos = float(the_sample.w > 0);
    float neg = float(the_sample.w <= 0);

    positive_ray_result += the_sample * pos;
    negative_ray_result += the_sample * neg;
    positive_counter += pos;
    negative_counter += neg;

    illumination_decay *= decay;
  }

  positive_ray_result /= (positive_counter > 0.0) ? positive_counter : 1.0;
  negative_ray_result /= (negative_counter > 0.0) ? negative_counter : 1.0;

  vec4 the_sample = sample_bilinear( orig_texcoord, src_size );

  result.xyz += positive_ray_result.xyz * positive_weight * num_samples;
  result.xyz += the_sample.xyz * ( negative_weight > 0.0 ? mix( 1.0, length( negative_ray_result.xyz ), negative_weight / 0.4 ) : 1.0 );
  //result.xyz += the_sample.xyz * negative_ray_result.xyz * negative_weight;

  result.xyz = dither( orig_texcoord * src_size, result.xyz );

  result.w = 1;

  imageStore( dst_tex, global_id, result );
}
