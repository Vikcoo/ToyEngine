#ifndef TE_RHI_DESCRIPTOR_BINDINGS_GLSL
#define TE_RHI_DESCRIPTOR_BINDINGS_GLSL

#if defined(TE_RHI_VULKAN)
    #define TE_RESOURCE_BINDING(groupIndex, bindingIndex) layout(set = groupIndex, binding = bindingIndex)
    #define TE_UNIFORM_BINDING(groupIndex, bindingIndex) layout(std140, set = groupIndex, binding = bindingIndex)
    #define TE_VERTEX_INDEX gl_VertexIndex
#else
    #define TE_RESOURCE_BINDING(groupIndex, bindingIndex) layout(binding = bindingIndex)
    #define TE_UNIFORM_BINDING(groupIndex, bindingIndex) layout(std140, binding = bindingIndex)
    #define TE_VERTEX_INDEX gl_VertexID
#endif

#endif
