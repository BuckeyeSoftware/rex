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
All rendering resources and commands happen through `Frontend::Context`. Every command on the frontend is associated with a tag that tracks the file and line information of the command in the engine as well as a static string describing it, this is provided to the interface with the `RX_RENDER_TAG("string")` macro.

The entire rendering interface is _thread safe_, any and all commands can be called from any thread at any time. With the exception of the `process` function which must be called on the main thread.

The frontend **does not** do immediate rendering. Every command executed is only recorded into a command buffer for later execution by the backend. There's some latency incurred by this, but it's also what permits thread-safety for APIs like OpenGL which cannot be called from multiple threads. The backend implements the `process` function and interprets commands. Every command is prefixed with a header including the data built by that `RX_RENDER_TAG` so it's very easy to see where in the engine a command originated from. These commands can be debugged with the in-engine renderer debugger.


#### Drawing
All drawing is done with a single function `Context::draw`.

Here's the definition:
```cpp
void draw(
  const CommandHeader::Info& _info,
  const State& _state,
  Target* _target,
  const Buffers& _draw_buffers,
  Buffer* _buffer,
  Program* _program,
  Size _count,
  Size _offset,
  Size _instances,
  Size _base_vertex,
  Size _base_instance,
  PrimitiveType _primitive_type,
  const Textures& _draw_textures);
```

Lets disect what everything is:
* `_info` is the render tag containing file, line and static string information as described at the beginning of this document. You construct one with `RX_RENDER_TAG(...)` macro.
* `_state` is the entire state vector to use. This is described [here]().
* `_target` is the render target to render into.
* `_draw_buffers` is the draw buffers in the target to use and how they're configured.
* `_buffer` is a combined vertex, element, and instance buffer to use for the draw.
* `_program` is the program to use.
* `_count` is the number of primitives to render.
* `_offset` is the offset in the element buffer to begin renering from. This should be 0 for non-indexed buffers.
* `_instances` is the number of instances to render. This should be 0 for non-instanced buffers.
* `_base_vertex` is the base vertex to add to each element. This should be 0 for non-indexed buffers.
* `_base_instance` is the base instance. This should be 0 for non-instanced buffers.
* `_primitive_type` is the primitive type to render, e.g `TRIANGLES`.
* `_draw_textures` is a description of textures to use in the draw.

When executed, primitives are assembled from the buffer `_buffer` using the primitive topology specified by `_primitive_type` and count `_count` consecutive vertex indices at offset `_offset` with the first vertex index value equal to `_base_vertex`. The primitives drawn `_instances` count times with the instance index starting with `_base_instance` and increasing sequentially for each instance. The assembled primitives execute the program `_program` and output to target `_target` configured by `_draw_buffers`. Textures bound for the program are configured by `_draw_textures`.

Assertions can be triggered in the following cases:

  * The viewport specified by state `_state` has zero-area.
  * The draw buffers are empty.
  * When `_program == nullptr`.
  * When `_count == 0`.
  * When `_offset != 0` when buffer-less.
  * When `_instances != 0` when buffer-less.
  * When `_base_vertex != 0` when buffer-less.
  * When `_base_instance != 0` when buffer-less.
  * When `_instances > 0` when `_buffer` is not instanced.
  * When `_offset > 0` when `_buffer` is not indexed.
  * When `_base_vertex > 0` when `_buffer` is not indexed.

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

When `RX_RENDER_CLEAR_STENCIL` is present in `_clear_mask`, the stencil clear value is expected as `Sint32` in one of two positions depending on if `RX_RENDER_CLEAR_DEPTH` is supplied. When `RX_RENDER_CLEAR_DEPTH` isn't supplied, the stencil clear value is expected in first position, otherwise it's expected in second position.

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
Size instanced_draw_calls() const;
Size clear_calls() const;
Size blit_calls() const;
Size vertices() const;
Size triangles() const;
Size lines() const;
Size points() const;
Size commands() const;
Size footprint() const;
Uint64 frame() const;
```

The `stats` function in particular can tell you how many objects you can have of that type; `total`, how many are currently in use; `used`, how many are cached; `cached` and how much memory (in bytes) is being used currently for those used objects _last_ frame.

The `draw_calls`, `instanced_draw_calls`, `clear_calls`, and `blit_calls` tell you how many draws, clears and blits happened _last_ frame.

The `vertices`, `triangles`, `lines`, and `points` tell you how many primitives were generated of each type _last frame_.

The `commands` function tells you how many frontend render commands were generated _last frame_.

The `footprint` function gives an estimate (in bytes) of how much memory the _last frame_ took in GPU bandwidth.

The `frame` function ggives you the frame number of the _last frame_.

In addition, timing information for a frame can be accessed with the following member function:

```cpp
const FrameTimer& timer() const &;
```

Which consists of information that can be queried with the following member functions:

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

Where `frame_times` here is an array of previous frame times used for computing histograms, averaging, etc.

### Resources
There's multiple resource types provided by the frontend, they're listed here.
  * Buffer
  * Target
  * Texture1D
  * Texture2D
  * Texture3D
  * TextureCM
  * Program

All resources are created by `Context::create_*()` functions, e.g `Context::create_buffer()` creates a buffer, likewise, all resources are destroyed by `Context::destroy_*()` functions.

Created resources are **not initialized** resources, instead you record information or request requirements of that resource through it's member functions and initialize the resource with `Context::initialize_*()` functions.
Once a resource is initialized it's properties are immutable, **you cannot reinitialize or respecifiy it**.

While resources cannot have their properties reinitialized or respecified, their contents can be updated. This is done with the `Context::update_*()` functions.

Every resource is validated when `Context::initialize_*()` is called. If at any point the resource is not fully specified (something was not recorded or requested), or an attempt was made to record a property or request a requirement that has already been recorded or requested, an assertion will be triggered. These assertions are disabled in release builds.

#### Buffer
A buffer resource represents a combined vertex, element, and instance buffer. Properties of the buffer are recorded with the following functions.
```cpp
// Record vertex attribute of |_type| starting at |_offset|.
void record_vertex_attribute(Attribute::Type _type, Size _offset);

// Record instance attribute of |_type| starting at |_offset|.
void record_instance_attribute(Attribute::Type _type, Size _offset);

// Record buffer type |_type|.
void record_type(Type _type);

// Record if instanced.
void record_instanced(bool _instanced);

// Record vertex stride |_stride|.
void record_vertex_stride(Size _stride);

// Record instance stride |_stride|.
void record_instance_stride(Size _stride);

// Record element format |_type|.
void record_element_type(ElementType _type);
```

The attributes type is listed here for completion sake to show that Rex does not use the usual offset, type, **count** tuple, but rather the type also implies the count.
```cpp
struct Attribute {
  enum class Type : Uint8 {
    // Scalars.
    F32,

    // Vectors.
    F32x2,
    F32x3,
    F32x4,
    S32x2,
    S32x3,
    S32x4,

    // Matrices.
    F32x4x4
  };

  Size offset;
  Type type;
};
```

The contents of the buffer can be initially specified or completely replaced by calling the following functions:
```cpp
// Write |_size| bytes from |_data| into store.
template<typename T>
void write_vertices(const T* _data, Size _size);
template<typename T>
void write_elements(const T* _data, Size _size);
template<typename T>
void write_instances(const T* _data, Size _size);
```

The contents of the buffer can be updated by mapping the store for each and updating the contents.
```cpp
// Map |_size| bytes from store.
Byte* map_vertices(Size _size);
Byte* map_elements(Size _size);
Byte* map_instances(Size _size);
```

When performing an update either by completely replacing the contents with `write_*` functions or updating through `map_` functions, the range of the edit needs to be recorded with the following functions:
```cpp
// Records an edit to the buffer at byte |_offset| of size |_size|.
void record_vertices_edit(Size _offset, Size _size);
void record_elements_edit(Size _offset, Size _size);
void record_instances_edit(Size _offset, Size _size);
```

> WARNING: Multiple buffer changes can be recorded by multiple calls to `record_*_edit`, however the contents of the edits are only visible to the backend once per frame. You cannot make edits between draw calls and expect those draw calls to see the changed contents. This invokes undefined behavior. The frontend will assert if it sees `update_buffer` for the same buffer more than once per frame.

Assertions can be triggered in the following cases:
* Not everything was recorded.
* The `map_vertices`, or `map_instances` functions are called with a size that is not a multiple of the recorded store stride.
* The `map_elements` function is called with a size that is not a multiple of the recorded element type's size.
* The `write_vertices`, `write_elements`, or `write_instances` functions are called with a size that is not a multiple of `sizeof(T)`.
* Any of the `write_*` or `map_*` functions are called when the recorded type is `k_static` after the buffer has been initialized.
* The `write_elements` or `map_elements` functions are called with a non-indexed buffer (`k_none` given to `record_type`.)
* The `write_instances` or `map_instances` functions are called with a non-instanced buffer (`false` given to `record_instanced`.)
* A vertex or instance attribute overlaps an existing attribute.
* A vertex or instance attribute exceeds the recorded stride.
* An attempt was made to modify the buffer after it was initialized.

#### Target
Targets are essentially frame buffer objects. They can contain multiple attachments.

Before initializing a target you can request it have and manage it's own textures as attachments or record your own, non-managed attachments from existing textures. This is down with the following member functions:
```cpp
// Request target have depth attachment |_format| with size |_dimensions|.
void request_depth(Texture::DataFormat _format,
                    const Math::Vec2z& _dimensions);

// Request target have stencil attachment |_format| with size |_dimensions|.
void request_stencil(Texture::DataFormat _format,
                     const Math::Vec2z& _dimensions);

// Request target have combined depth stencil attachment with size |_dimensions|.
void request_depth_stencil(Texture::DataFormat _format,
                           const Math::Vec2z& _dimensions);

// Attach existing depth texture |_depth| to target.
void attach_depth(Texture2D* _depth);

// Attach existing stencil texture |_stencil| to target.
void attach_stencil(Texture2D* _stencil);

// Attach existing depth stencil texture |_depth_stencil| to target.
void attach_depth_stencil(Texture2D* _depth_stencil);

// Attach texture |_texture| level |_level| to target.
void attach_texture(Texture2D* _texture, Size _level);

// Attach cubemap face |_face| texture |_texture| level |_level| to target.
void attach_texture(TextureCM* _texture, TextureCM::Face _face, Size _level);

// Attach cubemap texture |_texture| level |_level| to target, this attaches
// all faces of a cubemap in +x, -x, +y, -y, +z, -z order.
void attach_texture(TextureCM* _texture, Size _level);
```

Targets are immutable. Once initially specified they can no longer be updated.

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
1D, 2D, 3D and Cubemap textures are available in various capacities for texture sampling and rendering to (render to texture).

> NOTE: Only 2D and Cubemap textures can be used for drawing to.


```cpp
// Record the data format |_format|.
void record_format(DataFormat _format);

// Record the texture type |_type|.
void record_type(Type _type);

// Record the texture filter options |_options|.
void record_filter(const FilterOptions& _options);

// Record the number of levels |_levels|, including the base-level.
void record_levels(Size _levels);

// Record the optional border color |_color|.
void record_border(const Math::Vec4f& _color);
```

The contents of the texture can be specified and updated by calling the following functions:

```cpp
// Write data |_data| to store for level |_level|.
void write(const Byte* _data, Size _level);

// Map data for level |_level|.
Byte* map(Size _level);
```

> WARNING: Multiple textures changes can be recorded by multiple calls to `record_edit`, however the contents of the edits are only visible to the backend once per frame. You cannot make edits between draw calls and expect those draw calls to see the changed contents. This invokes undefined behavior. The frontend will assert if it sees `update_texture` for the same texture more than once per frame.

> NOTE: All levels _must_ be provided. There is no automatic derivation of miplevels in the frontend or backend. You may calculate the miplevels for a texture with `Texture::Chain`.

Assertions can be triggered in the following cases:
* Not all levels were written.
* A depth, stencil or combined depth-stencil format was recorded and `ATTACHMENT` was not used for type.
* An attempt was made to write or map a level when `ATTACHMENT` was recorded as the type.
* An attempt was made to write or map a level when `STATIC` was recorded as the type after initialization.
* An attempt was made to write or map a level that is out of bounds.
* An attempt was made to write or map a level other than zero for a texture with no mipmaps.
* An attempt was made to record dimensions that are greater than 4096 pixels in any dimension.

#### Program
This resource is not actually used directly. It's used to implement `Frontend::Technique` which provides specialization, variants and permutations.

It's listed here for completion sake.

Shaders and uniforms are added to a constructed program before initialization with the following member functions:

```cpp
// Add a shader definition |_shader|.
void add_shader(Shader&& shader_);

// Add a uniform with name |_name| and type |_type|.
Uniform* add_uniform(const String& _name, Uniform::Type _type);
```

Assertions can be triggered in the following cases:
* A shader was added that has already been added (e.g more than one vertex, fragment, compute, etc shader).
* A uniform was added that has already been added.

### State
All rendering state aside from bound resources is completely isolated into one single state vector that is passed around for draw calls. This vector is described by `Frontend::State` and consists of the following state vectors:

```
ScissorState scissor;
BlendState blend;
DepthState depth;
CullState cull;
StencilState stencil;
```

The state vector is hashed as well as the nested state objects to avoid excessive state changes in the backend. You do not need to manage any state when rendering, state bleed is not possible. This vector can be built every frame for every draw command.

### Technique

Techniques are [data-driven](https://en.wikipedia.org/wiki/Data-driven_programming) and described by JSON5. Information on how that's done is documented [here](TECHNIQUE.md)

Once a technique is loaded by `Frontend::Technique::load` you may fetch a program from that technique with the `operator Program*()`, `variant()` or `permute()` member functions depending on what is needed.

When getting a variant you pass an index of the variant you want to use. The variant used is the one listed in the `"variants"` array in the JSON5.

When getting a permute you pass the flags of the permutations you want to use. The flags are listed in the `"permutes"` array in the JSON5. If you pass a value of `(1 << 0) | (1 << 1)` as an example, then you're selecting permute 0 and 1 from the `"permutes"` list in the JSON5.

### Minimal fullscreen quad example
Here's a simple example of rendering a textured quad
```cpp
```

## Backend
While the frontend abstraction is used to isolate graphics API code from the actual engine rendering.
The backend abstraction is used to hook up that abstraction to the API code. This is done by `src/rx/render/backend`. The documentation of how this backend interface works is provided here to get you up to speed on how to write a backend.

The following backends already exist:
  * GL3 `src/rx/render/backend/gl3.{h,cpp}`
  * GL4 `src/rx/render/backend/gl4.{h,cpp}`
  * ES3 `src/rx/render/backend/es3.{h,cpp}`
  * NVN `src/rx/render/backend/nvn.{h,cpp}`
  * NULL `src/rx/render/backend/null.{h,cpp}`

### Command Buffer
The [frontend interface](#frontend-interface) allocates commands from a command buffer which are executed by the [backend interface](#backend-interface) `process()` function.

Each command has a 16-byte memory alignment and memory layout that includes a `CommandHeader`.

The lifetime of a command lasts for exactly one frame, for the command buffer is cleared by `Context::process` every frame.

Every command on the command buffer is prefixed with a command header which indicates the command type as well as an info object, called a tag that can be used to track where the command origniated from.

The command header looks like this.
```cpp
struct alignas(16) CommandHeader {
  struct Info {
    constexpr Info(const char *_description, const SourceLocation &_source_location)
      : description{_description}, source_info{_source_location} {
    }

    const char *description;
    SourceLocation source_info;
  };

  CommandType type;
  Info tag;
};
```

The `RX_RENDER_TAG(...)` macro is what constructs the `CommandHeader::Info` object which allows for file, line and a description string to be tracked through the frontend and the backend for debugging purposes. It's listed here for posterity sake.
```cpp
#define RX_RENDER_TAG(_description) \
  ::Rx::Render::Frontend::CommandHeader::Info{(_description), RX_SOURCE_LOCATION}
```

#### Commands
The following command types exist:

```cpp
enum class CommandType : Byte {
  RESOURCE_ALLOCATE,
  RESOURCE_CONSTRUCT,
  RESOURCE_UPDATE,
  RESOURCE_DESTROY,
  CLEAR,
  DRAW,
  BLIT,
  PROFILE
};
```

As well as their appropriate structures:

```cpp
struct DrawCommand {
  Buffers draw_buffers;
  Textures draw_textures;
  State render_state;
  Target *render_target;
  Buffer *render_buffer;
  Program *render_program;
  Size count;
  Size offset;
  Size instances;
  Size base_vertex;
  Size base_instance;
  PrimitiveType type;
  Uint64 dirty_uniforms_bitset;

  const Byte *uniforms() const;

  Byte *uniforms();
};

struct ClearCommand {
  Buffers draw_buffers;
  State render_state;
  Target *render_target;
  bool clear_depth;
  bool clear_stencil;
  Uint32 clear_colors;
  Uint8 stencil_value;
  Float32 depth_value;
  Math::Vec4f color_values[Buffers::k_max_buffers];
};

struct BlitCommand {
  State render_state;
  Target *src_target;
  Size src_attachment;
  Target *dst_target;
  Size dst_attachment;
};

struct ProfileCommand {
  const char *tag;
};

struct ResourceCommand {
  enum class Type : Uint8 {
    k_buffer,
    k_target,
    k_program,
    k_texture1D,
    k_texture2D,
    k_texture3D,
    k_textureCM
  };

  Type type;

  union {
    Target *as_target;
    Buffer *as_buffer;
    Program *as_program;
    Texture1D *as_texture1D;
    Texture2D *as_texture2D;
    Texture3D *as_texture3D;
    TextureCM *as_textureCM;
  };
};

struct UpdateCommand {
  enum class Type : Uint8 {
    k_buffer,
    k_texture1D,
    k_texture2D,
    k_texture3D
  };

  Type type;

  union {
    Buffer *as_buffer;
    Texture1D *as_texture1D;
    Texture2D *as_texture2D;
    Texture3D *as_texture3D;
  };

  // The number of edits to the resource in this update.
  Size edits;

  // The edit stream is an additional, variably-sized stream of data included as
  // a footer on this structure. It's contents encode a variable amount of edits
  // to the given resource.
  //
  // The encoding of the edit stream is a list of Size integers. The number
  // of integers per edit is determined by the resource Type |kind|.
  //
  // Buffer edits are represented by a three-tuple of integers of the format
  // {
  //   sink:   sink to edit: 0 = elements, 1 = vertices, 2 = instances
  //   offset: byte offset
  //   size:   size in bytes
  // }
  //
  // Texture edits are represented by a variable-tuple of integers of the format
  // {
  //   level:  miplevel to edit
  //   offset: offset in pixels (1, 2, or 3 integers for 1D, 2D and 3D textures, respectively)
  //   size:   size in pixels (1, 2, or 3 integers for 1D, 2D and 3D textures, respectively)
  // }
  const Size *edit() const;

  Size *edit();
};
```

All command structures, aside from `ResourceCommand`, include a `Frontend::State` object that represents the state vector to use for that operation.

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
