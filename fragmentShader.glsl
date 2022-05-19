#version 450
#extension GL_ARB_separate_shader_objects : enable

#define SAMPLES             512
#define ODE_SAMPLES         16
#define LIGHT_SAMPLES       0
#define EPS                 0.0001

#define ALPHA               0.0
#define X_0                 0.0
#define N0                  1.3
#define N1                  1.0
#define OMEGA               200.0

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec3 fragColor;
layout(location = 0) out vec4 outColor;

vec3 lastDir = vec3(0.0, 0.0, 0.0);

float sphereSDF(const in vec3 point, const in vec3 center, const in float radius) {
    return 1.0;
    return sign(length(point - center) - radius);
}

float cubeSDF(const in vec3 point, const in vec3 center, const in float side) {
    return sign(point.y - center.y);
    if (abs(point.x - center.x) < side*0.5 && abs(point.y - center.y) < side*0.5 && abs(point.z - center.z) < side*0.5) {
        return -1.0;
    }
    if (abs(point.x - center.x) == side*0.5 && abs(point.y - center.y) == side*0.5 && abs(point.z - center.z) == side*0.5) {
        return 0.0;
    }
    if (abs(point.x - center.x) > side*0.5 && abs(point.y - center.y) > side*0.5 && abs(point.z - center.z) > side*0.5) {
        return 1.0;
    }
}

float n(vec3 x) {
    return -(-0.8734845419052732 - 3.256636411883414 * x.y -0.184321985029691 * x.y * x.y + 1.6442425106833825*x.y*x.y*x.y);
    //return (N1 + N0) / 2.0 - (N1 - N0) / 2.0 * tanh(ALPHA * (x.y - X_0));
    return 1.0 + ALPHA * (x.y - X_0);
}

vec3 gradN(vec3 x) {
    return -vec3(0.0, -3.256636411883414 - 0.184321985029691*2.0*x.y + 3.0*1.6442425106833825*x.y*x.y, 0.0);
    //return vec3(0.0, (N0 - N1) / 2.0 * ALPHA / pow(cosh((x.y - X_0)*ALPHA), 2.0), 0.0);
    return vec3(0.0, ALPHA, 0.0);
}

vec3 getNextPoint(vec3 x0, vec3 dir0, float delta) {
    // TODO: rewrite without symmetry assumptions
    // first we need to rotate vector so it will be put in xz plane
    vec2 x = vec2(0, x0.y);
    vec2 dir = normalize(vec2(length(dir0.xz), dir0.y));
    float xt = dir.y / dir.x;
    // Euler
    float eps = abs(delta) / ODE_SAMPLES;
    dir = normalize(vec2(1.0, xt));
    for (float i = 0.0; i < ODE_SAMPLES; i += 1.0) {
        vec3 curp = vec3(x0.x + x.x*normalize(dir0.xz).x, x.y, x0.z + x.x*normalize(dir0.xz).y);
        vec3 norm = vec3(normalize(dir0.xz).x*dir.y, -dir.x, normalize(dir0.xz).y*dir.y);
        float k = abs(dot(gradN(curp), norm)) / n(curp) * pow(1 + xt * xt, 1.5);
        // k = 0.0;

        xt += sign(delta) * eps * k;
        x += sign(delta) * eps * vec2(1.0, xt);
        dir = normalize(vec2(1.0, xt));
    }
    lastDir = vec3(normalize(dir0.xz).x*dir.x, dir.y, normalize(dir0.xz).y*dir.x);
    vec3 test = vec3(x0.x + x.x*normalize(dir0.xz).x, x.y, x0.z + x.x*normalize(dir0.xz).y);
    vec3 linear = vec3(x0.x + dir0.x * delta, x0.y + dir0.y * delta, x0.z + dir0.z * delta);
    return vec3(test.x, test.y, test.z);

    //return vec3(x0.xy + x.x*normalize(dir0.xy), x.y);
}

void main() {
    mat4 uVirtualCameraModelMatrix = ubo.view; 
    vec3 v_ray = vec3(fragColor.xy * 0.5, fragColor.z);

    /*
    outColor = vec4(v_ray, 1.0);
    return;
    */

    vec4 ray = uVirtualCameraModelMatrix * vec4(normalize(v_ray), 0.0);
    vec4 origin = uVirtualCameraModelMatrix * vec4(vec3(0.0), 1.0);
    float maxDist = 10.0;
    float delta = maxDist / SAMPLES;

    vec3 lightColor1 = vec3(0.0, 1.0, 0.0);
    vec3 lightColor2 = vec3(1.0, 0.0, 0.0);

    float radius1 = 0.2;
    vec3 center1 = vec3(0.0,0.0,0.0);
    vec3 color1 = vec3(0.2, 0.0, 0.2);

    float radius2 = 0.2;
    vec3 center2 = vec3(0.5, 0.0, 0.0);
    vec3 color2 = vec3(0.0, 0.2, 0.2);

    vec3 dirLight1 = normalize(vec3(0.1, -0.2, 0.3));
    vec3 dirLight2 = normalize(vec3(0.1, 0.0, 0.0));

    vec3 currentPoint = origin.xyz;
    vec3 currentDir = ray.xyz;

    float sphereTest1 = cubeSDF(currentPoint, center1, radius1);
    float sphereTest2 = sphereSDF(currentPoint, center2, radius2);

    outColor = vec4(0.0, 0.0, 0.0, 1.0);

    for (float i = 0.0; i < SAMPLES; i += 1.0) {

        //currentPoint += delta * ray.xyz;
        currentPoint = getNextPoint(currentPoint, currentDir, delta);
        currentDir = lastDir;

        float currentTest1 = cubeSDF(currentPoint, center1, radius1);
        float currentTest2 = sphereSDF(currentPoint, center2, radius2);

        if (sphereTest1 != currentTest1) {

            delta *= -0.5;
            sphereTest1 = currentTest1;

            if (length(delta) < EPS) {
                vec3 norm = vec3(0.0, 0.0, 0.0);
                if (length(currentPoint.z - center1.z - radius1*0.5) < 5.0*EPS) {
                    norm.z += 1.0;
                }
                if (length(currentPoint.z - center1.z + radius1*0.5) < 5.0*EPS) {
                    norm.z -= 1.0;
                }
                if (length(currentPoint.y - center1.y - radius1*0.5) < 5.0*EPS) {
                    norm.y += 1.0;
                }
                if (length(currentPoint.y - center1.y + radius1*0.5) < 5.0*EPS) {
                    norm.y -= 1.0;
                }
                if (length(currentPoint.x - center1.x - radius1*0.5) < 5.0*EPS) {
                    norm.x += 1.0;
                }
                if (length(currentPoint.x - center1.x + radius1*0.5) < 5.0*EPS) {
                    norm.x -= 1.0;
                }
                norm = normalize(norm);

                vec3 cp = currentPoint;

                float light1_weight = 1.0;
                float light2_weight = 1.0;

                currentPoint = cp;
                delta = maxDist / LIGHT_SAMPLES;
                sphereTest2 = sphereSDF(currentPoint, center2, radius2);

                for (float j = 0.0; j < LIGHT_SAMPLES; j += 1.0) {

                    currentPoint -= delta * dirLight1;

                    float currentTest2 = sphereSDF(currentPoint, center2, radius2);

                    if (sphereTest2 != currentTest2) {

                        light1_weight = 0.0;
                        break;

                    }

                }

                currentPoint = cp;
                delta = maxDist / LIGHT_SAMPLES;
                sphereTest2 = sphereSDF(currentPoint, center2, radius2);

                for (float j = 0.0; j < LIGHT_SAMPLES; j += 1.0) {

                    currentPoint -= delta * dirLight2;

                    float currentTest2 = sphereSDF(currentPoint, center2, radius2);

                    if (sphereTest2 != currentTest2) {

                        light2_weight = 0.0;
                        break;

                    }

                }

                vec3 color = clamp(-dot(norm, dirLight1)*light1_weight, 0.0, 1.0)*lightColor1+clamp(-dot(norm,dirLight2)*light2_weight, 0.0, 1.0)*lightColor2+color1;
                /*
                if (cp.y < -1.0/ALPHA) {
                    color.b = 1.0;
                }
                if (-1.0/ALPHA < cp.y && cp.y < 0.0) {
                    color.r = 1.0;
                }
                */
                //color.r = (radius1+cp.y) / (2.0 * radius1);
                //color.b = 1.0 - color.r;
                //color.g = 0.0;
                
                color = vec3(1.0) * (0.5+0.5*sign(cos(length(cp) * OMEGA) * sin(2.0*atan(cp.z / cp.x))));
                //color = vec3(1.0) * (0.5+0.5*sign(sin(cp.x * OMEGA) * sin(cp.z * OMEGA))) / (length(cp) + 1.0);
                
                outColor = vec4(color, 1.0);
                break;
            } 

        }

        if (sphereTest2 != currentTest2) {

            delta *= -0.5;
            sphereTest2 = currentTest2;
        
            if (length(delta) < 0.0001) {
                vec3 norm = normalize(currentPoint - center2);

                vec3 cp = currentPoint;

                float light1_weight = 1.0;
                float light2_weight = 1.0;

                currentPoint = cp;
                delta = maxDist / LIGHT_SAMPLES;
                sphereTest1 = cubeSDF(currentPoint, center1, radius1);

                for (float j = 0.0; j < LIGHT_SAMPLES; j += 1.0) {

                    currentPoint -= delta * dirLight2;

                    float currentTest1 = cubeSDF(currentPoint, center1, radius1);

                    if (sphereTest1 != currentTest1) {

                        light2_weight = 0.0;
                        break;

                    }

                }

                currentPoint = cp;
                delta = maxDist / LIGHT_SAMPLES;
                sphereTest1 = cubeSDF(currentPoint, center1, radius1);

                for (float j = 0.0; j < LIGHT_SAMPLES; j += 1.0) {

                    currentPoint -= delta * dirLight1;

                    float currentTest1 = cubeSDF(currentPoint, center1, radius1);

                    if (sphereTest1 != currentTest1) {

                        light1_weight = 0.0;
                        break;

                     }

                }

                vec3 color = clamp(-dot(norm, dirLight1)*light1_weight, 0.0, 1.0)*lightColor1+clamp(-dot(norm,dirLight2)*light2_weight, 0.0, 1.0)*lightColor2+color2;
                outColor = vec4(color, 1.0);
                break;
            }

        }

    }

}
