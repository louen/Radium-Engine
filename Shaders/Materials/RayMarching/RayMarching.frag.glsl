struct Material
{
    isampler3D buffer;
};

uniform Material material;
out vec4 fragColor;

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_texcoord;
layout (location = 2) in vec3 in_viewVector;

float stepsize = 0.1;


vec4 shade(vec3 position, in int value) {
    if ( value > 1)
        return (vec4(0.1, 0., 0., 0.1));
}

void main(void) {
	int value;
	vec4 color;
	vec4 frgColor = vec4(0.0);

	float att=1.0;

	// le rayon
	vec3 position = in_texcoord;//in_position;
//	vec3 direction = - in_viewVector;
    //position.yz = vec2(0);

    value = texture(material.buffer, in_position).r;

    if (value == 0) fragColor = vec4(position,1.);//vec4(1.0, 0., 0., 1.0);
    else fragColor = vec4(0.0, 0., 1., 1.0);

//    for(int i = 0; i< int(4./stepsize); i++) {
//        value = texture(material.buffer, position).r;
//        if (value >= 1) {
//            color = shade(position, value); 
//            frgColor += color;
//            att = 0;
//            break;
//        }
//        position = position + direction * stepsize;
//    }

//    //if (att > 0.99) discard;
//    frgColor.a = 1.0;
//    fragColor = frgColor;
//    fragColor = vec4(1.0, 0., 0., 1.0);
}
