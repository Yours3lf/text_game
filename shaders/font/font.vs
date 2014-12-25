#version 430

layout(location=0) uniform mat4 mvp;

layout(location=0) in vec2 in_vertex;
layout(location=1) in vec2 in_texture;
layout(location=2) in vec4 instance_vertscalebias;
layout(location=3) in vec4 instance_texscalebias;
layout(location=4) in vec4 instance_color;
layout(location=6) in mat4 instance_transform;
layout(location=10) in float instance_filter;

out vec2 tex_coord;
flat out vec4 fontcolor;
flat out float filter_weight;

void main()
{
  filter_weight = instance_filter;
  fontcolor = instance_color;
  tex_coord = in_texture.xy * instance_texscalebias.xy + instance_texscalebias.zw;
  gl_Position = (mvp) * vec4((instance_transform * vec4(in_vertex.xy * instance_vertscalebias.xy, 0, 1)).xy + instance_vertscalebias.zw, 0, 1);
}
