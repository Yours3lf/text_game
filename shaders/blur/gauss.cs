#version 430 core

layout(binding=0, rgba8) readonly uniform image2D src_tex;
layout(binding=1) writeonly uniform image2D dst_tex;

layout(local_size_x = 16, local_size_y = 16) in; //local workgroup size

layout(location=0) uniform vec2 direction = vec2( 1, 0 );
layout(location=1) uniform float radius = 1.0;

//gauss weights set up for bilinear filtering
float weights[6] =
{
  0.0103814, 0.0944704, 0.296907, 0.296907, 0.0944704, 0.0103814
};

float offsets[6] =
{
  -5.17647, -3.29412, -1.41176, 1.41176, 3.29412, 5.17647
};

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

void main()
{
  ivec2 global_id = ivec2( gl_GlobalInvocationID.xy );
  ivec2 global_size = imageSize( dst_tex ).xy;
  ivec2 src_size = imageSize( src_tex ).xy;
  vec2 texcoord = vec2(global_id) / vec2(global_size);

  vec2 dir = direction / vec2(src_size);

  vec4 center_sample = imageLoad(src_tex, ivec2(texcoord * src_size));

  vec4 result = center_sample * 0.196483;
  float weight_sum = 0.196483;

  //bilateral gauss filter
  for(int c = 0; c < 6; c++)
  {
    result += sample_bilinear( texcoord + offsets[c] * dir * radius, src_size ) * weights[c];
    weight_sum += weights[c];
  }

  result *= 1.0 / weight_sum;

  imageStore( dst_tex, global_id, result );
}
