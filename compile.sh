cp vertexShader.glsl shader.vert
cp fragmentShader.glsl shader.frag
glslc shader.vert -o vert.spv
glslc shader.frag -o frag.spv
