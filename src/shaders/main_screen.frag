#version 430 core
uniform sampler2D sceneTex; // 0
uniform float vx_offset;
uniform float rt_w; // GeeXLab built-in
uniform float rt_h; // GeeXLab built-in
uniform float pixel_w = 15.0; // 15.0
uniform float pixel_h = 10.0; // 10.0
uniform bool noEffect;
uniform bool doPixelation;
uniform bool doOffset;
uniform bool doGrayscale;
uniform bool doCartoon;
uniform bool edgeFilter;
uniform vec2 offsets[9];
uniform int  edge_kernel[9];
in vec2 TexCoords;

void main() 
{
  vec2 uv = TexCoords.xy;
  vec3 tc = vec3(1.0, 0.0, 0.0);
  if(noEffect){
    tc = texture2D(sceneTex, uv).rgb;
  }

  if(doPixelation){
	if (uv.x < (vx_offset-0.005))
    {
      float dx = pixel_w*(1./rt_w);
      float dy = pixel_h*(1./rt_h);
      vec2 coord = vec2(dx*floor(uv.x/dx),
                        dy*floor(uv.y/dy));
      tc = texture2D(sceneTex, coord).rgb;
    }
    else if (uv.x>=(vx_offset+0.005))
    {
      tc = texture2D(sceneTex, uv).rgb;
    }
  }else{
    tc = texture2D(sceneTex, uv).rgb;
  }

  if(doOffset){
    float time = 1.0f;
    tc = texture( sceneTex, uv + 0.005*vec2( sin(time+1024.0*uv.x),cos(time+768.0*uv.y)) ).xyz;
  }

  if(doGrayscale){
    float average = 0.2126 * tc.r + 0.7152 * tc.g + 0.0722 * tc.b;
    tc = vec3(average,average,average);
  }

  if(doCartoon){
    tc = texture2D(sceneTex, uv).rgb;
    tc.r = int(tc.r * 255);
    tc.g = int(tc.g * 255);
    tc.b = int(tc.b * 255);
    int R_val = int(tc.r / 32) * 32;
    int G_val = int(tc.g / 32) * 32;
    int B_val = int(tc.b / 32) * 32;
    
    tc.r = float(R_val) / 255.0;
    tc.g = float(G_val) / 255.0;
    tc.b = float(B_val) / 255.0;
  }

  if(edgeFilter){
    vec3 s[9];
    for(int i = 0; i < 9; i++)
        s[i] = vec3(texture(sceneTex, TexCoords.st + offsets[i]));
    for(int i = 0; i < 9; i++)
        tc += vec3(s[i] * edge_kernel[i]);
  }

  gl_FragColor = vec4(tc, 1.0);
}