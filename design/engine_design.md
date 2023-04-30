# CoreGraphicsEngine
API-agnostic functionality lives here, along with (ideally) most of the state. Since it will interact with an opaque IGraphicsBackend, we can also have an easier time (and get more value out of) testing the functionality that lives here.

N.B. that we will make things slightly easier for us by exposing GLM outside of vkmisc, but we will still try to keep it as an implementation detail within the graphics namespace where we can...

```cpp
// API-agnostic vertex description; combines the concept of bindings (group of vertices) and attrs (single vertex)
VertexSpec {
  // vk::VertexInputAttributeDescription
  std::array {
    // U32 location; not needed since we can figure this out dynamically...?
    U32 binding
    U32 format
    U32 size // should this be a uniform element size?
    // U32 offset; offset not needed; since we can figure all of that out dynamically: let's make things easy and assume that the size of each vertex attrib will be constant, which will make offset easy to calculate.
  }

  // vk::VertexInputBindingDescription; N.B. no need to repeat binding here since this is a homogenous struct (is that a mistake?)
  U32 stride
  U32 input_rate
}

Vertex_t {
  static VertexSpec spec()

  ... members // MUST correspond to what's in the VertexSpec; use a static assert to enforce this.
}

IMesh {
  VertexSpec &spec() = 0;
  void *data() = 0
}

// Constrained to types that have a static spec() member that returns a VertexSpec
template<typename Vertex_t>
Mesh : IMesh{
  VertexSpec &spec = Vertex_t::spec()

  std::vector<Vertex_t> data

  Mesh(std::vector<Vertex_t> vertices)
  Mesh(std::vector<Vertex_t> &&vertices)

  // Factory to load from an obj file
  static Mesh load_obj()
}

Renderable {
  IMesh &mesh
  Matrix_t transform_matrix // an alias of glm... easier to just make glm a shared dependency btwn the App and the GraphicsAPI... :/ unless we can think of something cleaner?
  UID instanceId // unique per instance
  UID MeshId = TypeHash<Mesh<Vertex_t>>

  // TODO: shaders should own their push constants...
  Shader &vertexShader
  Shader &fragShader

  std::vector<PushConstantArbitrarySize> pushConstants;

  template<typename T>
  set_push_constant()
}
```

# VulkanBackend
An implementation of IGraphicsBackend. Since the abstractions from Vulkan are minimal to nonexistent, we don't need to bother with testing here, since we would just that would amount to testing the Vulkan implementation itself. Furthermore, this lack of testing (should) incentivize us to keep state to a minimum and allow most of it to live in the GraphicsEngine consuming this class (also promotes graphics API independence).
  - Takes in Renderables and manages Vk-specific handles for it
  - Take in a renderable, and create a Pipeline for it.
  - Renderables can be added more than once (e.g. w/ a different transform matrix; can be instanced later on).
  - Renderables using the same shaders and vertexspec and pipelinelayout (determined by push constants) can share the same pipeline!
  - Cache the pipeline for it
  - TODO: renderpasses/subpasses will prob factor into this somehow? e.g. we may also eventually want to pass a renderable with some kind of spec specifying how the VulkanBackend should setup renderpasses/subpasses for the corresponding pipeline... for now we will use a single set of renderpasses+subpasses


