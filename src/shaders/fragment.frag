#version 450
layout(location = 0) uniform int sampleUniform;
layout(location = 1) uniform vec2 songInfo;
layout(location = 2) uniform vec2 misc;
out vec4 o;

float map(vec3 p, float hit, float time)
{
    //vec3 q = fract(p) * 2.0 - 1.0;
	vec3 q = fract(p) * 2.0 - 1.0;
    return length(q) - 0.7 + (hit*2.0-1.0)/2.0;
}

float trace(vec3 o, vec3 r, float hit, float hit2, float time)
{
    float t = 0.0;
    for (int i=0; i < (32 - int(hit2 * 16)); i++) {
        vec3 p = o + r * t;
        float d = map(p, hit, time);
        t += d * 0.5;
    }
    return t;
}

float scale(int i)
{
	return float(i) / 255.0;
}

void main()
{
    float time = float(sampleUniform)/44100.0;
    vec2 res = vec2(1280,720);
	vec2 fragCoord = gl_FragCoord.xy;
	float kick = songInfo.x;
	float snare = songInfo.y;

    // Normalized pixel coordinates (from 0 to 1)
    vec2 uv = fragCoord.xy/res.xy;
    
    uv = uv * 2.0 - 1.0;
    
    uv.x *= res.x / res.y;
    
    vec3 ray = normalize(vec3(uv, 1.0));
    //vec3 origin = vec3(0.0, 0.0, -3.0);
    vec3 origin = vec3(0.0, 0.0, time);
    
    float t = trace(origin, ray, snare, kick, time);
    float fog = 1.0 / (1.0 + t * t * 0.1);
    vec3 fc = vec3(fog);

	vec3 normal = vec3(0.4, 0.1, 0.1);
	vec3 color = 0.5 + 0.5*cos(time/2.0+uv.xyx+vec3(0,2,4));

	float smallKick = kick * 0.5;
	vec3 effect = color * smallKick + normal * (1 - smallKick);
	//float effect = 1.0;
	
	float startFadeAt1 = 17.0; // s
	float fadeDuration1 = 0.5; // s
	float keepUntil1 = 19;
	float startFadeAt2 = 38.0; // s
	float fadeDuration2 = 3.0; // s

	float d = 0.0;
	float dimmer = 1.0;
	if (time > startFadeAt1 && time < keepUntil1) {
		d = time - startFadeAt1;
		dimmer = dimmer - abs(d/fadeDuration1);
	}
	if (time > startFadeAt2) {
		d = time - startFadeAt2;
		dimmer = dimmer - abs(d/fadeDuration2);
	}
	
	o = vec4(fc*effect, max(0.0, 1.0 - kick)) * dimmer;
	//o.x *= sin(misc.x);
	//o.y *= cos(misc.y);

    // Output to screen
    //fragColor = vec4(col,1.0);
}