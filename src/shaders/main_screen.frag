#version 330 core
uniform sampler2D sceneTex; // 0
uniform float vx_offset;
uniform float rt_w; // GeeXLab built-in
uniform float rt_h; // GeeXLab built-in
uniform float pixel_w = 15.0; // 15.0
uniform float pixel_h = 10.0; // 10.0
uniform bool doPixelation;
uniform bool doOffset;
uniform bool doGrayscale;
in vec2 TexCoords;

void main() 
{
  vec2 uv = TexCoords.xy;
  vec3 tc = vec3(1.0, 0.0, 0.0);
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

  gl_FragColor = vec4(tc, 1.0);
}