#version 330

//#version 450
//#version 300 es
// #version 330
//in highp vec4 vColor;

in highp vec3 vColor;
out highp vec4 fColor;

void main()
{
   fColor = vec4(vColor, 1.0);
}
