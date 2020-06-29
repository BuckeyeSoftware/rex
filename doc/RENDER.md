# Renderer
## Table of contents
  * [Renderer](#renderer)
    * [Frontend](#frontend)
        * [Interface](#frontend-interface)
          * [Drawing](#drawing)
          * [Clearing](#clearing)
          * [Blitting](#blitting)
          * [Swapchain](#swapchain)
          * [Queries](#queries)
        * [Resources](#resources)
          * [Buffer](#buffer)
          * [Target](#target)
          * [Texture](#texture)
          * [Program](#program)
        * [State](#state)
        * [Technique](#technique)
        * [Minimal fullscreen quad example](#minimal-fullscreen-quad-example)
    * [Backend](#backend)
        * [Command Buffer](#command-buffer)
          * [Commands](#commands)
        * [Interface](#backend-interface)

## Frontend
Rex employs a renderer abstraction interface to isolate graphics API code from the actual engine rendering. This is done by `src/rx/render/frontend`. The documentation of how this frontend interface works is provided here to get you up to speed on how to render things.

### Frontend Interface
All rendering resources and commands happen through `frontend::Context`. Every command on the frontend is associated with a tag that tracks the file and line information of the command in the engine as well as a static string describing it, this is provided to the interface with the `RX_RENDER_TAG("string")` macro.

The entire rendering interface is _thread safe_, any and all commands can be called from any thread at any time.

The frontend **does not** do immediate rendering. Every command executed is only recorded into a command buffer for later execution by the backend. There's exactly a frame of latency incurred by this but it's also what permits thread-safety for APIs like OpenGL which cannot be called from multiple threads. The backend implements the `process` function and interprets commands. Every command is prefixed with that `RX_RENDER_TAG` so it's very easy to see where in the engine a command originated from.

#### Drawing
Indexed and non-indexed draws are done with `frontend.draw`. When a buffer has no elements, e.g `element_kind == k_none` the draw is treated as a non-indexed draw.

Here's the definition:
```cpp
void draw(
  const CommandHeader::Info& _info,
  const State& _state,
  Target* _target,
  const char* _draw_buffers,
  Buffer* _buffer,
  Program* _program,
  Size _count,
  Size _offset,
  PrimitiveType _primitive_type,
  const char* _textures,
  ...);
```

Lets disect what everything is:
* `_info` is the render tag containing file, line and static string information as described at the beginning of this document. You construct one with `RX_RENDER_TAG(...)`.
* `_state` is the entire state vector to use.
* `_target` is the render target to render into.
* `_draw_buffers` is the draw buffer specification to use (described below.)
* `_buffer` is the combined vertex and element buffer to source.
* `_program` is the program to use.
* `_count` is the number of elements to source.
* `_offset` is the offset in the element buffer to begin renering from.
* `_primitive_type` is the primitive type to render, e.g `k_triangles`.
* `_textures` is the texture specification to use (described below.)
* `...` is a list of `texture{1D, 2D, 3D, CM}` to use for the draw.

The texture specification string is a string-literal encoding the textures to use for this call using the following table
  * `1` 1D texture
  * `2` 2D texture
  * `3` 3D texture
  * `c` Cubemap texture

The string can have a maximum length of eight, meaning you can have a maximum amount of eight textures. There must be exactly as many textures passed in `...` as this string's length. They must mach the specification, e.g

The draw buffer specification string is a string-literal encoding the attachments to use for this draw and in which order those attachments should be configured as draw buffers. The string can have a maximum of eight characters.

#### Clearing
Clearing of a render target is done by `Context::clear`, here's the definition:

```cpp
  void clear(
    const CommandHeader::Info& _info,
    const State& _state,
    Target* _target,
    const char* _draw_buffers,
    Uint32 _clear_mask,
    ...
  );
```

* `_info` is a `RX_RENDER_TAG(...)`.
* `_state` is the state vector to use.
* `_target` is the target to clear.
* `_draw_buffers` is the draw buffer specification.
* `_clear_mask` is a combination of bitor clear options.
* `...` is the clear packet data

Performs a clear operation on `_target` with specified draw buffer layout `_draw_buffers` and state `_state`. The clear mask specified by `clear_mask` describes the packet layout of `...`.

The packet data described in `...` is passed, parsed and interpreted in the following order:
  * depth:   `Float64` _truncated to `Float32`_
  * stencil: `Sint32`
  * colors:  `const Float32*`

When `RX_RENDER_CLEAR_DEPTH` is present in `_clear_mask`, the depth clear value is expected as `Float64` (truncated to `Float32`) in first position.

When `RX_RENDER_CLEAR_STENCIL` is present in `_clear_mask|`, the stencil clear value is expected as `Sint32` in one of two positions depending on if `RX_RENDER_CLEAR_DEPTH` is supplied. When `RX_RENDER_CLEAR_DEPTH` isn't supplied, the stencil clear value is expected in first position, otherwise it's expected in second position.

When `RX_RENDER_CLEAR_COLOR(n)` for any `n` is present in `_clear_mask`, the clear value is expected as a pointer to `Float32` (`const Float32*`) containing four color values in normalized range in RGBA order. The `n` refers to the index in the `_draw_buffers` specification to clear. The association of the clear value in `...` and the `n` is done in order. When a `RX_RENDER_CLEAR_COLOR` does not exist for a given `n`, the one proceeding it takes it's place, thus gaps are skipped.

#### Blitting
Blitting of a render target is done by `Context::blit`, here's the definition:

```cpp
void blit(
  const CommandHeader::Info& _info,
  Target* _src_target,
  Size _src_attachment,
  Target* _dst_target,
  Size _dst_attachment
);
```

* `_info` is a `RX_RENDER_TAG(...)`
* `_state` is the entire state vector to use.
* `_src_target` is the source target to blit _from_.
* `_src_attachment` is the source attachment to use for the _read_.
* `_dst_target` is the destination target to blit _to_.
* `_dst_attachment` is the destination attachment to use for the _write_.

#### Swapchain
The swapchain target is exposed by `Context::swapchain()`.

This target is special in that it's the target used to render to the window or display.

It has exactly one color attachment that represents the window or display's back buffer.

#### Queries
You may query information about the renderer with the following member functions:

```cpp
struct Statistics {
  Size total;
  Size used;
  Size cached;
  Size memory;
};

Statistics stats(Resource::Type _type) const;
Size draw_calls() const;
Size clear_calls() const;
Size blit_calls() const;
Size vertices() const;
Size triangles() const;
Size lines() const;
Size points() const;
```

The `stats` function in particular can tell you how many objects you can have of that type; `total`, how many are currently in use; `used`, how many are cached; `cached` and how much memory (in bytes) is being used currently for those used objects _last_ frame.

The `draw_calls`, `clear_calls` and `blit_calls` tell you how many draws, clears and blits happened last frame.

The `vertices`, `triangles`, `lines` and `points` tell you how many primitives were generated of each type last frame.

In addition, timing information for a frame can be accessed with the following member function:

```cpp
const FrameTimer& timer() const &;
```

Which consists of a lot of information that can be queried with the following member functions:

```cpp
Float32 mspf() const;
Uint32 fps() const;
Float32 delta_time() const;
Float64 resolution() const;
Uint64 ticks() const;

struct FrameTime {
  Float64 life;
  Float64 frame;
};

const Vector<FrameTime>& frame_times() const &;
```

### Resources
There's multiple resource types provided by the frontend, they're listed here.
  * buffer
  * target
  * texture1D
  * texture2D
  * texture3D
  * textureCM
  * program

All resources are created by `Context::create_*()` functions, e.g `Context::create_buffer()` creates a buffer, likewise, all resources are destroyed by `Context::destroy_*()` functions.

Created resources are **not initialized** resources, instead you record information or request requirements of that resource through it's member functions and initialize the resource with `Context::initialize_*()` functions.
Once a resource is initialized it's properties are immutable, **you cannot reinitialize or respecifiy it**.

While resources cannot have their properties reinitialized or respecified, their contents can be updated. This is done with the `Context::update_*()` functions.

Every resource is validated when `Context::initialize_*()` is called. If at any point the resource is not fully specified (something was not recorded or requested), or an attempt was made to record a property or request a requirement that has already been recorded or requested, an assertion will be triggered. These assertions are disabled in release builds.

#### Buffer
A buffer resource represents a combined vertex and element buffer for geometry. The properties that **must be** recorded are provided by the following:
```cpp
// record vertex attributes
// the order of calls is the order of the attributes
// |_type| is the attribute type (e.g float, int)
// |_count| is the # of elements of the attribute type (3 for a vec3)
// |_offset| is the offset in the vertex format this attribute begins in bytes
void record_attribute(Attribute::Type _type, Size _count, Size _offset);

// record vertex stride
// |_stride| is the size of the vertex format in bytes
void record_stride(Size _stride);

// record element type
// |_type| is the element type, k_none for no elements
void record_element_type(ElementType _type);

// record type |_type|
// k_static is for buffers that will not be updated
// k_dynamic is for buffers that will be updated
void record_type(Type _type);
```

The contents of the buffer can be specified and updated by calling the following functions:
```cpp
// write |_size| bytes from |_data| into vertex store
template<typename T>
void write_vertices(const T* _data, Size _size);

// write |_size| bytes from |_data| into element store
template<typename T>
void write_elements(const T* _data, Size _size);

// map |_size| bytes of vertices
Byte* map_vertices(Size _size);

// map |_size| bytes of elements
Byte* map_elements(Size _size);
```

Assertions can be triggered in the following cases:
* Not everything was recorded
* The `map_vertices` function is called with a size that is not a multiple of the recorded vertex stride.
* The `map_elements` function is called with a size that is not a multiple of the recorded element type's byte size.
* The `write_vertices` or `write_elements` functions are called with a size that is not a multiple of `sizeof(T)`.
* The `write_vertices`, `write_elements`, `map_vertices`, or `map_elements` functions are called with a recorded type that is `k_static` after the buffer has been initialized.
* The `write_elements` or `map_elements` functions are called with a recorded element type `k_none`.
* An attribute overlaps an existing attribute.
* An attribute exceeds the recorded vertex stride.
* An attempt was made to modify the buffer after it was initialized.

#### Target
Targets are essentially frame buffer objects. They can contain multiple attachments.

Before initializing a target you request certain requirements. Those requirements are requested with the following member functions:

```cpp
// request the swap chain for this target
// you cannot request or attach anything if you request this
void request_swapchain();

// request target have depth attachment |_format| with size |_dimensions|
void request_depth(Texture::DataFormat _format,
  const math::Vec2z& _dimensions);

// request target have stencil attachment |_format| with size |_dimensions|
void request_stencil(Texture::DataFormat _format,
  const math::Vec2z& _dimensions);

// request target have combined depth stencil attachment with size |_dimensions|
void request_depth_stencil(Texture::DataFormat _format,
  const math::Vec2z& _dimensions);
```

A target can also be attached existing textures with the following member functions:

```cpp
// attach existing depth texture |_depth| to target
// can only attach one depth texture
void attach_depth(Texture2D* _depth);

// attach existing stencil texture |_stencil| to target
// can only attach one stencil texture
void attach_stencil(Texture2D* _stencil);

// attach texture |_texture| to target
// the order this function is called is the order the textures are attached
void attach_texture(Texture2D* _texture);
```

Targets are immutable. Once initially specified they can no longer be changed.

Assertions can be triggered in the following cases:
* An attempt was made to attach a texture that has already been attached.
* An attempt was made to attach more than one depth, stencil or combined depth stencil texture.
* An attempt was made to attach a non-depth and non-stencil texture which has different dimensions than the other attached non-depth and non-stencil textures.
* An attempt was made to attach a non-depth and non-stencil texture which has a different format than the other attached non-depth and non-stencil textures.
* An attempt was made to attach a depth, stencil or combined depth-stencil texture that has a recorded type which is not `k_attachment`.
* An attempt was made to attach a depth texture when a stencil texture is already attached, you **must use combined depth-stencil** texture instead.
* An attempt was made to attach a stencil texture when a depth texture is already attached, you **must use combined depth-stencil** texture instead.
* An attempt was made to modify the target after it was initialized.

#### Texture
1D, 2D, 3D and Cubemap textures are used either for drawing to (if attached to a target) or for actual texture mapping during rendering.

Before you can initialize a texture certain information must be recorded by the following member functions:

```cpp
// record format |_format|
void record_format(DataFormat _format);

// record type
// |_type| can be one of k_static, k_dynamic, k_attachment
// k_static is for textures that will not be updated
// k_dynamic is for textures that will be updated
// k_attachment is for textures that will only be used as target attachments
void record_type(Type _type);

// record filter options |_options|
void record_filter(const FilterOptions& _options);

// record dimensions |_dimensions|
void record_dimensions(const DimensionType& _dimensions);

// record wrapping behavior |_wrap|
void record_wrap(const WrapOptions& _wrap);
```

The contents of the texture can be specified and updated by calling the following functions:

```cpp
// write data |_data| to store for miplevel |_level|
void write(const Byte* _data, Size _level);

// map data for miplevel |_level|
Byte* map(Size _level);
```

Assertions can be triggered in the following cases:
* Not all miplevels were written.
* A depth, stencil or combined depth-stencil format was recorded and `k_attachment` was not used for type.
* An attempt was made to write or map a miplevel when `k_attachment` was recorded as the type.
* An attempt was made to write or map a miplevel when `k_static` was recorded as the type after initialization.
* An attempt was made to write or map a miplevel that is out of bounds.
* An attempt was made to write or map a miplevel other than zero for a texture with no mipmaps.
* An attempt was made to record dimensions that are greater than 4096 pixels in any dimension.
* An attempt was made to modify the texture after it was initialized.

All miplevels _must_ be provided to the render resource. There is no automatic derivation of miplevels in the render frontend or backend. You may derive the miplevels for a texture with `texture::chain` interface instead.

#### Program
This resource is not actually used directly. It's used to implement `frontend::Technique` which provides specialization, variants and program permutations.

It's listed here for completion sake.

Shaders and uniforms are added to a constructed program before initialization with the following member functions:

```cpp
// add a shader definition |_shader|
void add_shader(Shader&& shader_);

// add a uniform with name |_name| and type |_type|
Uniform& add_uniform(const String& _name, Uniform::Type _type);
```

Assertions can be triggered in the following cases:
* A shader was added that has already been added (e.g more than one vertex, fragment, compute, etc shader).
* A uniform was added that has already been added.
* An attempt was made to modify the program after it was initialized.


### State
All rendering state aside from bound resources is completely isolated into one single state vector that is passed around for draw calls. This vector is described by `frontend::State` and consists of the following state:

```
ScissorState scissor;
BlendState blend;
DepthState depth;
CullState cull;
StencilState stencil;
PolygonState polygon;
```

The state vector is hashed as well as the nested state objects to avoid excessive state changes in the backend. You do not need to manage any state when rendering, state bleed is not possible. This vector can be built every frame for every draw command.

### Technique

Techniques are [data-driven](https://en.wikipedia.org/wiki/Data-driven_programming) and described by JSON5. Information on how that's done is documented [here](TECHNIQUE.md)

Once a technique is loaded by `frontend::Technique::load` you may fetch a program from that technique with the `operator Program*()`, `variant()` or `permute()` member functions depending on what is needed.

When getting a variant you pass an index of the variant you want to use. The variant used is the one listed in the `"variants"` array in the JSON5.

When getting a permute you pass the flags of the permutations you want to use. The flags are listed in the `"permutes"` array in the JSON5. If you pass a value of `(1 << 0) | (1 << 1)` as an example, then you're selecting permute 0 and 1 from the `"permutes"` list in the JSON5.

### Minimal fullscreen quad example
Here's a simple example of rendering a textured quad
```cpp
// vertex format
struct QuadVertex {
  rx::math::Vec2f position;
  rx::math::Vec2f coordinate;
};

// vertices of our quad
static constexpr const QuadVertex k_quad_vertices[]{
  {{ -1.0f,  1.0f}, {0.0f, 1.0f}},
  {{  1.0f,  1.0f}, {1.0f, 1.0f}},
  {{ -1.0f, -1.0f}, {0.0f, 0.0f}},
  {{  1.0f, -1.0f}, {1.0f, 0.0f}}
};

// elements of our quad
static constexpr const Byte k_quad_elements[]{
  0, 1, 2, 3
};

// create a buffer
rx::render::frontend::Buffer* quad = frontend.create_buffer(RX_RENDER_TAG("quad"));

// the contents will not change
quad->record_type(rx::render::frontend::Buffer::Type::k_static);

// record the element format
quad->record_element_type(rx::render::frontend::Buffer::ElementType::k_u8);

// record the vertex format
quad->record_stride(sizeof(QuadVertex));

// record the attributes
quad->record_attribute(rx::render::frontend::Buffer::Attribute::Type::k_f32, 2,
  offsetof(QuadVertex, position));
quad->record_attribute(rx::render::frontend::Buffer::Attribute::Type::k_f32, 2,
  offsetof(QuadVertex, coordinate);

// write the vertices and elements into the buffer
quad->write_vertices(k_quad_vertices, sizeof k_quad_vertices);
quad->write_elements(k_quad_elements, sizeof k_quad_elements);

// initialize it
frontend.initialize_buffer(RX_RENDER_TAG("quad"), quad);

// at this point nothing can be changed, we also cannot change the contents
// because we used k_static for our buffer type

// clear swapchain target with red
frontend.clear(RX_RENDER_TAG("test"),
  {},
  frontend.swapchain(),
  RX_RENDER_CLEAR_COLOR(0),
  {1.0f, 0.0f, 0.0f, 1.0f});

// assume we have some program handle here, left out for brevity
rx::render::frontend::Program* program{...};
// ...
frontend.initialize_program(RX_RENER_TAG("quad"), program);

// assume we have some texture2D handle here, left out for brevity
rx::render::frontend::Texture2D* texture{...};
// ...
frontend.initialize_texture(RX_RENER_TAG("quad"), texture);

// we can record changes to the program here before
// a draw call, e.g program->uniforms()[index]->set_float(1.0f)...
//
// the contents are _copied_ into the draw command when do you this

// draw the textured quad to the swapchain
frontend.draw(
  RX_RENDER_TAG("test"),
  state,
  frontend.swapchain(),
  quad,
  program,
  4,
  0,
  rx::render::frontend::PrimitiveType::k_triangle_strip,
  "2" // since 2D texture
  texture);
```

## Backend
While the frontend abstraction is used to isolate graphics API code from the actual engine rendering.
The backend abstraction is used to hook up that abstraction to the API code. This is done by `src/rx/render/backend`. The documentation of how this backend interface works is provided here to get you up to speed on how to write a backend.

The following backends already exist:
  * GL3 `src/rx/render/backend/gl3.{h,cpp}`
  * GL4 `src/rx/render/backend/gl4.{h,cpp}`
  * ES3 `src/rx/render/backend/es3.{h,cpp}`
  * NVN `src/rx/render/backend/nvn.{h,cpp}`

### Command Buffer
The [frontend interface](#frontend-interface) allocates commands from a command buffer which are executed by the [backend interface](#backend-interface) `process()` function.

Each command has a 16-byte memory alignment and memory layout that includes a `CommandHeader`.

The lifetime of a command lasts for exactly one frame, for the command buffer is cleared by `Context::process` every frame.

Every command on the command buffer is prefixed with a command header which indicates the command type as well as an info object, called a tag that can be used to track where the command origniated from.

The command header looks like this:

```cpp
struct alignas(16) CommandHeader {
  struct Info {
    constexpr Info(const char* _file, const char* _description, int _line)
      : file{_file}
      , description{_description}
      , line{_line}
    {
    }
    const char* file;
    const char* description;
    int line;
  };

  CommandType type;
  Info tag;
};
```

The `RX_RENDER_TAG(...)` macro is what constructs the `CommandHeader::Info` object which allows for file, line and a description string to be tracked through the frontend and the backend for debugging purposes.

#### Commands
The following command types exist:

```cpp
enum class CommandType : Byte {
  k_resource_allocate,
  k_resource_construct,
  k_resource_update,
  k_resource_destroy,
  k_clear,
  k_draw,
  k_blit
};
```

As well as their appropriate structures:

```cpp
struct ResourceCommand {
  enum class Type : Byte {
    k_buffer,
    k_target,
    k_program,
    k_texture1D,
    k_texture2D,
    k_texture3D,
    k_textureCM
  };

  Type kind;

  union {
    Target* as_target;
    Buffer* as_buffer;
    Program* as_program;
    Texture1D* as_texture1D;
    Texture2D* as_texture2D;
    Texture3D* as_texture3D;
    TextureCM* as_textureCM;
  };
};

struct ClearCommand : State {
  Target* render_target;
  int clear_mask;
  math::Vec4f clear_color;
};

struct BlitCommand : State {
  Target* src_target;
  Size src_attachment;
  Target* dst_target;
  Size dst_attachment;
};

struct DrawCommand : State {
  Target* render_target;
  Buffer* render_buffer;
  Program* render_program;
  Size count;
  Size offset;
  char texture_types[8];
  void* texture_binds[8];
  PrimitiveType type;
  Uint64 dirty_uniforms_bitset;

  const Byte* uniforms() const;
  Byte* uniforms();
};
```

All command structures, aside from `ResourceCommand`, include a `frontend::State` object that represents the state vector to use for that operation.

The `ResourceCommand` structure is used by all the resource commands and merely represents a tagged union of which resource to `allocate`, `construct`, `update`, or `destroy`.

The `DrawCommand` is special in that it's size is actually variable. The `frontend::Context::draw` function over allocates `DrawCommand` from the command buffer to make room to store the raw data of any changed uniforms of `render_program`. The member function `uniforms()` actually returns a pointer to `this + 1` to reach into that memory. The `dirty_uniform_bitset` has a set bit for each uniform in `render_program` that has changed. The index of these set bits are the uniform indices in `render_program`. The sizes of each uniform can be read from there as well as their type.
The uniform data in `DrawCommand` is tightly packed. An example of unpacking this data is provided:

```cpp
if (command->dirty_uniforms_bitset) {
  const auto& program_uniforms = render_program->uniforms();
  const Byte* draw_uniforms = command->uniforms();

  for (Size i = 0; i < 64; i++) {
    if (command->dirty_uniforms_bitset & (1_u64 << i)) {
      const auto& uniform = program_uniforms[i];

      // data for uniform is in draw_uniforms

      // bump the pointer along to the next changed uniform's data
      draw_uniforms += uniform.size();
    }
  }
}
```

### Backend Interface
Rendering backends implement a small interface which looks like:

```cpp
// sizes of resources reported by the backend
struct AllocationInfo {
  Size buffer_size;
  Size target_size;
  Size program_size;
  Size texture1D_size;
  Size texture2D_size;
  Size texture3D_size;
  Size textureCM_size;
};

struct DeviceInfo {
  const char* vendor;
  const char* renderer;
  const char* version;
};

struct Context {
  RX_MARK_INTERFACE(Context);

  virtual AllocationInfo query_allocation_info() const = 0;
  virtual DeviceInfo query_device_info() const = 0;
  virtual bool init() = 0;
  virtual void process(const Vector<Byte*>& _commands) = 0;
  virtual void swap() = 0;
};
```

The purposes of `query_allocation_info()` is to allow the frontend to over allocate objects on the render pool so that frontend objects have a 1:1 mapping with backend objects in memory. Or rather, taking the `this` pointer from a frontend object and adding one to it will get you to the memory of the backend object. This is also how virtual functions are avoided for rendering objects.

The `init()` function implements the _initialization_ of the backend. It should return `false` on failure. Do not do initialization work in the constructor since there's no way to indicate errors as exceptions are not used in Rex.

The `process(const Vector<Byte*>& _commands)` function implements the processing of commands as mentioned above. One call is made for every frame.

The `swap()` function is used to swap the swapchain.
