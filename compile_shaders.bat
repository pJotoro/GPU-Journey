set out_dir=%1
glslc shader.vert -o %out_dir%\vert.spv
glslc shader.frag -o %out_dir%\frag.spv