#version 430 core

#define PI 3.1415926538

#define MAX_TEXTURES 3
#define MAX_MATERIALS 5

#define MAX_SCENE_BOUNDS 1000.0

#define MAX_TRIS 20
#define MAX_SPHERES 50
#define MAX_PLANES 1

#define MAX_LIGHTS 4

#define RAY_STEPS 5000
#define RAY_MAX_DIST 1000.0f
#define EPSILON 0.001f

#define INVALID_TYPE -1
#define TRI_TYPE 0
#define SPHERE_TYPE 1
#define PLANE_TYPE 2

//NOTE ALL UBO-RELATED STRUCTS USE VEC4 IN PLACE OF VEC3

struct material {
    vec4 difCol;
    vec4 specCol;
    vec4 reflCol;
    vec4 transpCol;

    float shininess;
    float refrIndx;
    
    int texIndx;
};

struct sphere {
    vec4 center;
    float radius;
    int matIndx;
};

struct tri {
    vec4 a;
    vec4 ab;
    vec4 ac;
    vec4 n;

    //texture info
    vec2 tex0;
    vec2 tex1;
    vec2 tex2;

    int matIndx;
};

struct plane {
    vec4 p;
    vec4 n;
    vec4 u;
    vec4 v;

    int matIndx;
};

//per-intersection information (as opposed to per-object info that can be inferred later)
struct intersInfo {
    float dist;
    vec3 p;
    //reminder: n cannot necessarily be inferred from the object later
    vec3 n;
    vec2 txt;
};

struct closestInfo {
    float dist;
    //triangle, sphere, etc.
    int objType;
    //index within array of respective type
    int indx;
    intersInfo inters;
};

out vec4 FragColor;

//in vec2 pos;

uniform samplerCube skybox;
layout(location = 2) uniform sampler2D texs [MAX_TEXTURES];

layout(std140) uniform Unifs {
    tri tris [MAX_TRIS];
    sphere spheres [MAX_SPHERES];
    plane planes [MAX_PLANES];

    vec4 lights [MAX_LIGHTS];

    material maters [MAX_MATERIALS];

    vec4 ambLight;
    vec4 bgCol;

    vec4 eye;
    vec4 ray00;
    vec4 ray01;
    vec4 ray10;
    vec4 ray11;

    int num_tris;
    int num_spheres;
    int num_planes;
    int num_lights;

    float time;

    //float _pad[3];
};

//**************UTIL**********************
mat4 rotationMatrix(vec3 axis, float angle) {
    axis = normalize(axis);
    float s = sin(angle);
    float c = cos(angle);
    float oc = 1.0 - c;
    
    return mat4(oc * axis.x * axis.x + c,           oc * axis.x * axis.y - axis.z * s,  oc * axis.z * axis.x + axis.y * s,  0.0,
                oc * axis.x * axis.y + axis.z * s,  oc * axis.y * axis.y + c,           oc * axis.y * axis.z - axis.x * s,  0.0,
                oc * axis.z * axis.x - axis.y * s,  oc * axis.y * axis.z + axis.x * s,  oc * axis.z * axis.z + c,           0.0,
                0.0,                                0.0,                                0.0,                                1.0);
}

float PHI_R = 1.61803398874989484820459 * 00000.1; // Golden Ratio   
float PI_R  = 3.14159265358979323846264 * 00000.1; // PI
float SRT_R = 1.41421356237309504880169 * 10000.0; // Square Root of Two

float antialiasingOffset=450.0;

vec2 seed;

float getRandom2(int i) {
    return fract(sin(dot((10*i + antialiasingOffset) * seed, vec2(PHI_R, PI_R))) * SRT_R);
}

float getRandom(int i){
    return fract(sin(dot(vec2(seed.x + i, seed.y), vec2(12.9898,78.233))) * 43758.5453);
}

//**************RAY TRACING***************
bool intersectTri(tri t, vec3 origin, vec3 dir, inout intersInfo inters) {

    vec3 n = t.n.xyz;

    float n_dir = dot(n, dir);
    
    //don't worry about side-on intersections (explicitly check to avoid divide by zero later)
    if(abs(n_dir) < EPSILON)
        return false;
    
	vec3 bc = inverse(mat3(t.ab.xyz, t.ac.xyz, -dir)) * (origin - t.a.xyz);

    if((bc.z >= 0.0) && (0.0 <= bc.x) && (bc.x <= 1.0) && (0.0 <= bc.y) && (bc.y <= 1.0) && (bc.x + bc.y <= 1.0)) {
        inters.dist = bc.z;
        inters.p = origin + bc.z*dir;
        inters.n = n_dir < 0 ? n : -n;
        inters.txt = (1.0 - bc.x - bc.y) * t.tex0 + bc.x * t.tex1 + bc.y * t.tex2;
        return true;
    }
    return false;
}

bool intersectTris(vec3 origin, vec3 dir, inout closestInfo closest) {
    bool found = false;
    for (int i = 0; i < num_tris; i++) {
        intersInfo inters;
        if(intersectTri(tris[i], origin, dir, inters) && inters.dist < closest.dist) {
            closest.inters = inters;
            closest.dist = inters.dist;
            closest.objType = TRI_TYPE;
            closest.indx = i;
            found = true;
        }
    }
    return found;
}

bool intersectSphere(sphere s, vec3 origin, vec3 dir, inout intersInfo inters) {
    vec3 dif = origin - s.center.xyz;
    float a = dot(dir, dir);
    float b = 2.0 * dot(dif, dir);
    float c = dot(dif, dif) - s.radius * s.radius;
    float d = b * b - 4.0 * a * c;
    
    if(d >= 0.0)
    {
        d = sqrt(d);
        float t0 = 0.5 * (-b - d) / a;
        float t1 = 0.5 * (-b + d) / a;
        
        if(t0 >= 0.0) 
            inters.dist = t0;
        else if(t1 >= 0.0)
            inters.dist = t1;
        else
            return false;
            
        inters.p = origin + inters.dist*dir;
        inters.n = normalize(inters.p - s.center.xyz);

        if(length(dif) < s.radius)
            inters.n = -inters.n;

        //now calculate texture coordinate
        float phi = acos(inters.n.z);
        vec3 proj = normalize(vec3(inters.n.xy, 0.0));
        float theta = acos(proj.x);

        if(proj.y < 0)
            theta = 2.0 * PI - theta;

        inters.txt = vec2(theta / (2.0 * PI), phi / PI);

        return true;
    }
    
    return false;
}

bool intersectSpheres(vec3 origin, vec3 dir, inout closestInfo closest) {
    bool found = false;
    for (int i = 0; i < num_spheres; i++) {
        intersInfo inters;
        if(intersectSphere(spheres[i], origin, dir, inters) && inters.dist < closest.dist) {
            closest.inters = inters;
            closest.dist = inters.dist;
            closest.objType = SPHERE_TYPE;
            closest.indx = i;
            found = true;
        }
    }
    return found;
}

bool intersectPlane(plane p, vec3 origin, vec3 dir, inout intersInfo inters) {
    vec3 point = p.p.xyz;
    vec3 n = p.n.xyz;

    vec3 dif = origin - point;
    float perp = dot(dif, n);
    float dirn = dot(dir, n);

    if(perp * dirn < 0) {
        inters.dist = -perp / dirn;
        inters.p = origin + inters.dist*dir;
        inters.n = n;
        //can use dif instead of inters.p - point
        inters.txt = fract(dif.xz/100.0);
        return true;
    }
    return false;
}

bool intersectPlanes(vec3 origin, vec3 dir, inout closestInfo closest) {
    bool found = false;
    for (int i = 0; i < num_planes; i++) {
        intersInfo inters;
        if(intersectPlane(planes[i], origin, dir, inters) && inters.dist < closest.dist) {
            closest.inters = inters;
            closest.dist = inters.dist;
            closest.objType = PLANE_TYPE;
            closest.indx = i;
            found = true;
        }
    }
    return found;
}

void getClosest(vec3 origin, vec3 dir, inout closestInfo closest) {
    closest.dist = MAX_SCENE_BOUNDS;
    closest.indx = -1;
    intersectTris(origin, dir, closest);
    intersectSpheres(origin, dir, closest);
    intersectPlanes(origin, dir, closest);
}

vec4 trace(vec3 origin, vec3 dir, int depth) {
    vec4 mulAcc = vec4(1, 1, 1, 1);
    vec4 sum = vec4(0, 0, 0, 0);

    for(int i=0; i<depth; i++) {
        closestInfo closest;
        getClosest(origin, dir, closest);
        if (closest.indx != -1) {
            int mindx = -1;
            switch(closest.objType) {
                case TRI_TYPE:
                    mindx = tris[closest.indx].matIndx;
                break;
        
                case SPHERE_TYPE:
                    mindx = spheres[closest.indx].matIndx;
                break;
        
                case PLANE_TYPE:
                    mindx = planes[closest.indx].matIndx;
                break;
        
                default:
                    return vec4(0, 1, 0, 1);
                break;
            }
            material m = maters[mindx];

            vec4 texCol = (m.texIndx==-1) ? vec4(1, 1, 1, 1) : texture(texs[m.texIndx], closest.inters.txt);
            mulAcc *= texCol;

            vec4 res = m.difCol*ambLight;//*texCol;
    
            vec3 n = closest.inters.n;
            float cos_theta0 = dot(closest.inters.n, dir);
            float ri = m.refrIndx;

            if(cos_theta0 > 0) {
                n = -n;
            } else {
                cos_theta0 = -cos_theta0;
                ri = 1 / ri;
            }
            
            vec3 p = closest.inters.p + EPSILON*closest.inters.n;

            for(int i=0; i < num_lights; i++) {
                vec3 lpos = lights[i].xyz;
                vec3 ldir = lpos - p;
                float r = length(ldir);
                ldir = normalize(ldir);

                closestInfo shadow;
                getClosest(p, ldir, shadow);
                if(shadow.indx==-1 || shadow.dist > r) {
                    //res += 0.1 * m.difCol*max(0.0, dot(ldir, n)) / (0.002*r + 0.005*r*r);
                    res += 0.1f * m.difCol*max(0.0, dot(ldir, n)) / (0.002*r + 0.005*r*r);
                    //res += m.specCol*pow(abs(dot(normalize(ldir - dir), n)), m.shininess) / (0.005*r + 0.005*r*r);
                    res += m.specCol*pow(abs(dot(normalize(ldir - dir), n)), m.shininess) / (0.005*r + 0.005*r*r);
                } 
            }

            sum += mulAcc*res;

            //NOTE: assumes transmission and reflection are uniform across their respective rgb channels 
            float transpVal = m.transpCol.x;
            float reflVal =  m.reflCol.x;
            //if(getRandom(i) > transpMag / (transpMag + reflMag))  
            //float r0 = pow((1 - ri) / (1 + ri), 2);
            //float refcoeff = 1 - r0 - (1 - r0) * pow(1 - cos_theta0, 5);
            float rand = getRandom(i);

            /*if (reflVal + transpVal < EPSILON) {
                return vec4(sum.rgb, 1);
                //return vec4((sum + mulAcc*res).rgb, 1);
            } else /**/
            if (rand <= reflVal) {
                /* WORKING REFLECTION CODE */
                //setup ray continuation 
                dir = normalize(dir - 2 * dot(dir, n)*n);
                origin = closest.inters.p + EPSILON*dir;//p;
                mulAcc *= m.reflCol;
            } else if (rand <= reflVal + transpVal) {
                // ** EXPERIMENTAL REFRACTION CODE
                float cos_theta1 = sqrt(1 - (1 - cos_theta0 * cos_theta0) / ri / ri);
                dir = normalize(dir / ri - n * (cos_theta1 - cos_theta0 / ri));// / ri;
                //dir = (dir - n * (cos_theta1 - cos_theta0) )/ ri;
                origin = closest.inters.p + EPSILON*dir;
                mulAcc *= m.transpCol;
                // ** END
            } else {
               // ** EXPERIMENTAL BRDF
               vec3 n_orth = normalize(cross(cross(dir, n), n));
               //float rand_theta = 2 * PI * getRandom(2*i + 30);
               float rand1 = 2*getRandom(3*i + 200) - 1;
               float rand2 = getRandom(3*i + 10);
               dir = normalize(rand1*n_orth + rand2*n);
               //dir = mat3(rotationMatrix(n, rand_theta)) * normalize(dir*rand2 + n*(1 - rand2));
               origin = closest.inters.p + EPSILON*dir;
               mulAcc *= m.specCol; //?NEEDED?
               return vec4(sum.rgb, 1);
               //return vec4(sum.rgb, 1);//vec4((sum + mulAcc*res).rgb, 1);//vec4(sum.rgb, 1);
            }

            //return vec4(abs(n), 1);

        } else {
            //return sum + mulAcc*vec4(texture(skybox, dir).rgb, 1);
            return vec4(sum.rgb + mulAcc.rgb*texture(skybox, dir).rgb, 1);
            //return vec4(dir, 1);//vec4(texture(skybox, vec3(1.0, 0, 0)).rgb, 1.0);
            //return texture(skybox, -dir);
            //return sum + mulAcc*bgCol;
        }
    }
    return vec4(sum.rgb, 1);
}

//**************MAIN***************
layout(points) in;
layout(points, max_vertices = 1) out;
void main(void) {
  //ivec2 pix = ivec2(gl_FragCoord.xy) + ivec2(1, 1);
  //ivec2 size = imageSize(framebuffer).xy;
  //ivec2 size = ivec2(2, 2);
  //vec2 pix = 0.5 * gl_Position.xy + vec2(0.5, 0.5);
  //seed = pix;//1.0 * pix.xy / size.xy;
  //if (pix.x >= size.x || pix.y >= size.y){
  //  return;
  //}
  //vec2 pos = pix;//vec2(pix.xy) / vec2(size.x - 1, size.y - 1);
  vec2 pos = 0.5 * gl_in[0].gl_Position.xy + vec2(0.5, 0.5);
  pos = vec2(pos.x, 1 - pos.y);

  seed = pos;
  vec3 dir = mix(mix(ray00, ray01, pos.y), mix(ray10, ray11, pos.y), pos.x).xyz;
  vec4 color = vec4(0, 0, 0, 0);
  int samples = 30;
  for(int i=0; i<samples; i++) {
     color += trace(eye.xyz, dir, 3);
     seed = fract(seed + vec2(0.123644, 0.12847));
  }

  color /= color.w;
  //color = vec4(1, 0, 0, 1);

  //color *= vec4(texture(skybox, dir).rgb, 1.0);//trace(eye.xyz, dir, 2);
  //vec4 color2 = vec4(texture(skybox, dir).rgb, 1.0);
  //color += vec4(1, 0, 0, 1) * 
  //vec4 color = vec4(normalize(eye.xyz), 1);
  //imageStore(framebuffer, pix, color);
  FragColor = color;
  gl_Position = gl_in[0].gl_Position;
  EmitVertex();
  EndPrimitive();
}
