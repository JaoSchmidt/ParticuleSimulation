#shader vertex
#version 400 core

// Texture.glsl
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoord;
layout(location = 2) in vec4 a_Color;
layout(location = 3) in float a_TexIndex;
layout(location = 4) in float a_TilingFactor;

uniform mat4 u_ViewProjection;

out vec2 v_TexCoord;
out vec4 v_Color;
out float v_TexIndex;
out float v_TilingFactor;

void main()
{
  v_Color = a_Color;
  v_TexCoord = a_TexCoord;
  v_TexIndex = a_TexIndex;
  v_TilingFactor = a_TilingFactor;
  gl_Position = u_ViewProjection * vec4(a_Position, 1.0);
}

#shader fragment
#version 400 core

layout(location = 0) out vec4 color;
in vec4 v_Color;
in vec2 v_TexCoord;
// in float v_TexIndex;
// in float v_TilingFactor;

uniform sampler2D u_Textures[32];

void main()
{
  // --- texture from Texture.glsl ---
  // vec4 texColor =
  //   texture(u_Textures[int(v_TexIndex)],
  //     v_TexCoord * v_TilingFactor);

  // --- circle mask from Circles.glsl ---
  vec2 p = v_TexCoord * 2.0 - 1.0;
  float coverage = 1.0 - step(1.0, length(p));
  float alpha = v_Color.a * coverage;

  // ==== COMBINATIONS - uncomment ONE ====

  // [1] TINT (multiply) - circle color tints texture, white v_Color = raw texture
  // color = vec4(texColor.rgb * v_Color.rgb, texColor.a * alpha);

  // [2] ALPHA MASK only - circle color ignored, texture just clipped
  // color = vec4(texColor.rgb, texColor.a * alpha);

  // [3] PURE CIRCLE - texture ignored, solid v_Color circle (Circles.glsl)
  color = vec4(v_Color.rgb, alpha);

  // [4] MIX half/half - texture and circle color averaged
  // color = vec4(mix(texColor.rgb, v_Color.rgb, 0.5), texColor.a * alpha);

  // [5] ADDITIVE - texture + circle color (can exceed 1.0, clamps to white)
  // color = vec4(texColor.rgb + v_Color.rgb, texColor.a * alpha);

  // [6] SCREEN - 1-(1-tex)*(1-v), brightens toward white
  // color = vec4(vec3(1.0) - (vec3(1.0) - texColor.rgb) * (vec3(1.0) - v_Color.rgb), texColor.a * alpha);

  // [7] FULL MULTIPLY - v_Color fully modulates texture (RGB + A), then circle clip
  // color = texColor * v_Color;
  // color.a *= alpha;
}
