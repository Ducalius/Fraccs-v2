#version 330 core
#extension GL_ARB_gpu_shader_fp64 : enable
out vec4 FragColor;
in vec2 RawPos;

uniform float maxiter = 50;
uniform bool msaa = false;
uniform bool trap_bitmap = false;
uniform int deco = 2;
uniform vec2 resolution;
uniform double zoom;
uniform dvec2 center;
uniform dvec2 julia_point;

uniform sampler2D colormap;
uniform sampler2D texture2;

double magn = 0;

const dvec2 point = dvec2(-3.0, -3.0);

int mandel(dvec2 Pos, float l_maxiter) {
	dvec2 z = dvec2(0.0, 0.0);
	for(int i = 0; i < l_maxiter; i++) {
		z = dvec2(
			z.x * z.x - z.y * z.y + Pos.x,
			2 * z.x * z.y + Pos.y
		);

		if (z.x * z.x + z.y * z.y > 4.0f) {
			magn = sqrt(z.x * z.x + z.y * z.y);
			return i;
		}
	}
	magn = sqrt(z.x * z.x + z.y * z.y);
	return 0;
}

float trapmandel(dvec2 Pos, float l_maxiter, dvec2 Point) {

	float dist = 1e20;
	dvec2 z = dvec2(0.0, 0.0);
	bool escaped = false;
	for(int i = 0; i < l_maxiter; i++) {
		z = dvec2(
			z.x * z.x - z.y * z.y + Pos.x,
			2 * z.x * z.y + Pos.y
		);
		dvec2 minus_point = dvec2(z.x - Point.x, z.y - Point.y);

		double minus_point_magn = sqrt(minus_point.x * minus_point.x + minus_point.y * minus_point.y);
		if(minus_point_magn < dist)
			dist = float(minus_point_magn);
		
		if (z.x * z.x + z.y * z.y > 4.0f) {
			escaped = true;
		}
	}
	magn = sqrt(z.x * z.x + z.y * z.y);
	return escaped ? dist : 0;

}


float julia(dvec2 Pos, float l_maxiter, dvec2 Point) {
	dvec2 z = dvec2(Pos.x, Pos.y);
	for(int i = 0; i < l_maxiter; i++) {
		z = dvec2(
			z.x * z.x - z.y * z.y + Point.x,
			2 * z.x * z.y + Point.y
		);

		if (z.x * z.x + z.y * z.y > 4.0f) {
			magn = sqrt(z.x * z.x + z.y * z.y);
			return i;
		}
	}
	magn = sqrt(z.x * z.x + z.y * z.y);
	return 0;
}

bool escaped = false;
int iter;

vec4 bitmandel(dvec2 Pos, float l_maxiter) {
	dvec2 z = dvec2(0.0, 0.0);
	vec4 local_res = vec4(0.0);
	for(int i = 0; i < l_maxiter; i++) {
		z = dvec2(
			z.x * z.x - z.y * z.y + Pos.x,
			2 * z.x * z.y + Pos.y
		);

		local_res = texture(texture2, clamp(vec2(z), 0.0f, 1.0f)); // + (msaa ? vec4(0.2,0.2,0.2,0) : vec4(0.0));
		
		if (z.x * z.x + z.y * z.y > 4.0f || local_res.a > 0.1f) {
			magn = sqrt(z.x * z.x + z.y * z.y);
			escaped = true;
			iter = i;
			
			return local_res;
		}
		
	}
	magn = sqrt(z.x * z.x + z.y * z.y);
	iter = int(maxiter);
	return vec4(0.0);
}


void main() {
	

	dvec2 PlanePos = dvec2(
								RawPos.x / (resolution.x/resolution.y) / zoom + center.x,
								RawPos.y / zoom + center.y
							  ) * 2;

	//Invert Plane
	//double sum = PlanePos.x * PlanePos.x + PlanePos.y * PlanePos.y;
	//PlanePos = dvec2(PlanePos.x/sum, PlanePos.y/sum);

	float res;
	vec4 b_res;
	if (trap_bitmap)
		b_res = bitmandel(PlanePos, maxiter);
	else
		res = mandel(PlanePos, maxiter);
	double av_magn = magn;

	if(msaa){
		dvec2 offset = dvec2(
								2.0 / resolution.x / (resolution.x/resolution.y) / zoom,
								2.0 / resolution.y / zoom
							) / 2;
							
		dvec2 Poses[4];
		Poses[0] = PlanePos - offset;
		Poses[1] = PlanePos + offset;
		Poses[2] = dvec2(PlanePos.x - offset.x, PlanePos.y + offset.y);
		Poses[3] = dvec2(PlanePos.x + offset.x, PlanePos.y - offset.y);
		

		for(int j = 0; j < 3; j++) {
			if (trap_bitmap)
				b_res += bitmandel(Poses[j], maxiter);
			else
				res += mandel(Poses[j], maxiter);
			av_magn += magn;

		}

		if (trap_bitmap)
			b_res = b_res/5;
		else
			res = res/5;
		av_magn = av_magn/5;
		
	}

	//Normal coloring
	if (!trap_bitmap) {
		float smoonth = 1 - log( log(float(av_magn)) / log(2) ) / log(2);
		float final = res + smoonth;
		float factor = clamp( ( final / maxiter )*1.5, 0.0f, 1.0f);
		FragColor = (res == 0 && av_magn <= 2) ? vec4(0.15f, 0.0f, 0.15f, 1.0f) : vec4( factor + 0.2f, factor, 0.2f, 1.0f);
	
	//res += 1 - log( log(float(magn)) / log(2) ) / log(2);
	//double col = res / maxiter;
	//FragColor = vec4( col+ 0.2f, col, 0.2f, 1.0f);

	//FragColor = texture(colormap, vec2(col, 1.0f));
	//FragColor = res;
	//vec4 bg = texture(colormap, vec2((iter + smoonth)/ maxiter), 1.0f);

	} else {

		//Bitmap trapping
	
		float smoonth = 1 - log( log(float(av_magn)) / log(2) ) / log(2);
		float factor = clamp( ( (iter + smoonth) / maxiter) * 2, 0.0f, 1.0f);

		// Green 
		//vec4 bg = vec4( factor+0.1f, factor+0.1f, 0.2f, 1.0f);

		// Purple/Yellow
		vec4 bg = vec4( factor+ 0.2f, factor, 0.2f, 1.0f); 

		FragColor = (escaped ? mix(bg, b_res, b_res.a) : b_res);
	}

}