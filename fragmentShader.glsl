#version 450
#extension GL_ARB_separate_shader_objects : enable

#define SAMPLES             256
#define LIGHT_SAMPLES       64

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec3 fragColor;
layout(location = 0) out vec4 outColor;

float sphereSDF(const in vec3 point, const in vec3 center, const in float radius) {
    return sign(length(point - center) - radius);
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

    float radius1 = 0.1;
    vec3 center1 = vec3(0.0,0.0,0.0);
    vec3 color1 = vec3(0.2, 0.0, 0.2);

    float radius2 = 0.2;
    vec3 center2 = vec3(0.5, 0.0, 0.0);
    vec3 color2 = vec3(0.0, 0.2, 0.2);

    vec3 dirLight1 = normalize(vec3(0.1, 0.2, 0.3));
    vec3 dirLight2 = normalize(vec3(0.1, 0.0, 0.0));

    vec3 currentPoint = origin.xyz;

    float sphereTest1 = sphereSDF(currentPoint, center1, radius1);
    float sphereTest2 = sphereSDF(currentPoint, center2, radius2);

    outColor = vec4(0.0, 0.0, 0.0, 1.0);

    for (float i = 0.0; i < SAMPLES; i += 1.0) {

        currentPoint += delta * ray.xyz;

        float currentTest1 = sphereSDF(currentPoint, center1, radius1);
        float currentTest2 = sphereSDF(currentPoint, center2, radius2);

        if (sphereTest1 != currentTest1) {

            delta *= -0.5;
            sphereTest1 = currentTest1;

            if (length(delta) < 0.0001) {
                vec3 norm = normalize(currentPoint - center1);

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
                sphereTest1 = sphereSDF(currentPoint, center1, radius1);

                for (float j = 0.0; j < LIGHT_SAMPLES; j += 1.0) {

                    currentPoint -= delta * dirLight2;

                    float currentTest1 = sphereSDF(currentPoint, center1, radius1);

                    if (sphereTest1 != currentTest1) {

                        light2_weight = 0.0;
                        break;

                    }

                }

                currentPoint = cp;
                delta = maxDist / LIGHT_SAMPLES;
                sphereTest1 = sphereSDF(currentPoint, center1, radius1);

                for (float j = 0.0; j < LIGHT_SAMPLES; j += 1.0) {

                    currentPoint -= delta * dirLight1;

                    float currentTest1 = sphereSDF(currentPoint, center1, radius1);

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
