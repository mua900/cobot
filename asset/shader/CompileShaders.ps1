dxc -spirv -T vs_6_0 -E main src/vertex.hlsl -Fo binary/vertex.spv
dxc -spirv -T ps_6_0 -E main src/fragment.hlsl -Fo binary/fragment.spv
dxc -T vs_6_0 -E main src/vertex.hlsl -Fo binary/vertex.dxil
dxc -T ps_6_0 -E main src/fragment.hlsl -Fo binary/fragment.dxil
